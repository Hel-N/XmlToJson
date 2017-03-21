#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <Windows.h>
#include <vector>
#include <map>
#include <cstring>
#include <string>
#include <locale.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
//#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include "json_parser.hpp"

using namespace std;
using boost::property_tree::ptree;
using boost::property_tree::basic_ptree;

string inFileName = "yandex_log.xml";
string outFileName = "Output.json.txt";
string contestName;
int freezeTimeMinutesFromStart;
vector<string> problemLetters;
map<string, string> contestants;
ptree root_contest;

struct run{
	string contestantId;
	string problemTitle;
	int contestTime;
	bool verdict;
};

vector<run> allruns;

void NameOfFiles(int argc, char* argv[]); // получение имен файлов из входных параметров main

string narrow_string(wstring const &s, locale const &loc, char default_char); //перевод wstring в string

char const *russian_locale_designator = "rus"; // обозначение локали зависит от реализации, "rus" подходит для VC++

string string_for_write(string st);

void ReadAndParseXml(string filename);
void MakeAndWriteJson(string filename);


int main(int argc, char* argv[]){
	/*#ifdef _DEBUG
		freopen("Output.json.txt", "w", stdout);
		#endif*/

	setlocale(LC_ALL, "Rus");

	NameOfFiles(argc, argv);

	ReadAndParseXml(inFileName);
	MakeAndWriteJson(outFileName);

	return 0;
}

void NameOfFiles(int argc, char* argv[]){
	if (argc > 1){
		string stmp = argv[1];
		if (stmp != "-in" && stmp != "-out"){
			cout << "Параметры введены неверно!";
			exit(1);
		}
	}

	bool flag;
	for (int i = 1; i < argc; ++i){
		string stmp = argv[i];
		if (stmp == "-in"){
			flag = true;
			inFileName = "";
			continue;
		}
		if (stmp == "-out"){
			flag = false;
			outFileName = "";
			continue;
		}

		if (flag == true) inFileName += stmp;
		if (flag == false) outFileName += stmp;
	}

	if (inFileName == "" || outFileName == ""){
		cout << "Параметры введены неверно!";
		exit(1);
	}
}

string narrow_string(wstring const &s, locale const &loc, char default_char = '?')
{
	if (s.empty())
		return std::string();
	std::ctype<wchar_t> const &facet = std::use_facet<std::ctype<wchar_t> >(loc);
	wchar_t const *first = s.c_str();
	wchar_t const *last = first + s.size();
	std::vector<char> result(s.size());

	facet.narrow(first, last, default_char, &result[0]);

	return std::string(result.begin(), result.end());
}

string string_for_write(string st){
	return '"' + st + '"';
}

void ReadAndParseXml(string filename){
	string stmp;
	wchar_t wbuff[500];

	boost::property_tree::ptree propertyTree;

	try{
		//Читаем XML
		boost::property_tree::read_xml(filename, propertyTree);
	}
	catch (...){
		cout << "Ошибка чтения из файла!" << endl;
		exit(1);
	}

	locale loc(russian_locale_designator);

	try{
		//--->contestName
		stmp = propertyTree.get<string>("contestLog.settings.contestName");
		MultiByteToWideChar(CP_UTF8, 0, stmp.c_str(), 500, wbuff, 500);
		wstring ws = wbuff;
		contestName = narrow_string(ws, loc);

		//--->freezeTimeMinutesFromStart
		freezeTimeMinutesFromStart = 240;

		//--->problemLetters
		BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.problems"))
		{
			problemLetters.push_back(v.second.get<string>("<xmlattr>.title"));
		}

		//--->contestants
		BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.users"))
		{
			try{
				v.second.get<string>("<xmlattr>.participationType");
			}
			catch (...){
				stmp = v.second.get<string>("<xmlattr>.displayedName");
				MultiByteToWideChar(CP_UTF8, 0, stmp.c_str(), 500, wbuff, 500);
				wstring ws1 = wbuff;
				contestants[v.second.get<string>("<xmlattr>.id")] = narrow_string(ws1, loc);
			}
		}

		//--->runs
		BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.events"))
		{
			if (v.first == "submit"){
				string userid = v.second.get<string>("<xmlattr>.userId");
				if (contestants.count(userid) > 0)
				{
					run newRun;

					newRun.contestantId = userid;
					newRun.problemTitle = v.second.get<string>("<xmlattr>.problemTitle");
					newRun.contestTime = v.second.get<int>("<xmlattr>.contestTime") / 60000;
					newRun.verdict = v.second.get<string>("<xmlattr>.verdict") == "OK" ? true : false;

					allruns.push_back(newRun);
				}
			}
		}
	}
	catch (...){
		cout << "Ошибка получения данных!" << endl;
		exit(1);
	}
}

void MakeAndWriteJson(string filename){
	string stmp;
	try{
		//--->contestName
		root_contest.put<string>("contestName", string_for_write(contestName));

		//--->freezeTimeMinutesFromStart
		root_contest.put<int>("freezeTimeMinutesFromStart", freezeTimeMinutesFromStart);

		//--->problemLetters
		ptree  problems;

		for (auto it = problemLetters.begin(); it != problemLetters.end(); ++it){
			ptree pr;
			pr.put("", string_for_write(*it));
			problems.push_back(make_pair("", pr));
		}

		root_contest.put_child("problemLetters", problems);

		//--->contestants
		ptree  contestant;

		for (auto it = contestants.begin(); it != contestants.end(); ++it){
			ptree pr;
			pr.put("", string_for_write((*it).second));
			contestant.push_back(make_pair("", pr));

		}

		root_contest.put_child("contestants", contestant);

		//--->runs
		ptree runs;

		for (auto it = allruns.begin(); it != allruns.end(); ++it){
			ptree elem;

			elem.put<string>("contestant", string_for_write(contestants[(*it).contestantId]));
			elem.put<string>("problemLetter", string_for_write((*it).problemTitle));
			elem.put<int>("timeMinutesFromStart", (*it).contestTime);
			elem.put<bool>("success", (*it).verdict);

			runs.push_back(make_pair("", elem));
		}

		root_contest.put_child("runs", runs);
	}
	catch (...){
		cout << "Ошибка формирования структуры данных!" << endl;
		exit(1);
	}

	try{
		write_json(filename, root_contest);
	}
	catch (...){
		cout << "Ошибка записи в файл!" << endl;
		exit(1);
	}
}