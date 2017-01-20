#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdio.h>
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
#include <boost/algorithm/string/replace.hpp>
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
	bool samplesPassed;
};

vector<run> allruns;

string narrow_string(wstring const &s, locale const &loc, char default_char); //перевод wstring в string

char const *russian_locale_designator = "rus"; // обозначение локали зависит от реализации, "rus" подходит для VC++

string string_for_write(string st);

void ReadAndParseXml(string filename);
void MakeAndWriteJson(string filename);


int main(){
#ifdef _DEBUG
	freopen("Output.json.txt", "w", stdout);
#endif

	setlocale(LC_ALL, "Rus");

	ReadAndParseXml(inFileName);
	MakeAndWriteJson(outFileName);

	return 0;
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
	//Читаем XML
	boost::property_tree::read_xml(filename, propertyTree);

	locale loc(russian_locale_designator);

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
			ws = wbuff;
			contestants[v.second.get<string>("<xmlattr>.id")] = narrow_string(ws, loc);
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
				newRun.samplesPassed = v.second.get<bool>("<xmlattr>.samplesPassed");

				allruns.push_back(newRun);
			}
		}
	}
}

void MakeAndWriteJson(string filename){
	string stmp;

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
		elem.put<bool>("success", (*it).samplesPassed);

		runs.push_back(make_pair("", elem));
	}

	root_contest.put_child("runs", runs);

	write_json(filename, root_contest);
}