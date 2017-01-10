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
#include <boost/foreach.hpp>

using namespace std;


std::string narrow_string(std::wstring const &s, std::locale const &loc, char default_char = '?')
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

char const *russian_locale_designator = "rus"; // обозначение локали зависит от реализации,
// "rus" подходит для VC++

int main(){
#ifdef _DEBUG
	freopen("Output.json.txt", "w", stdout);
#endif

	setlocale(LC_ALL, "Rus");

	string filename = "yandex_log.xml";
	string stmp;
	wchar_t wbuff[500];
	map<string, string> contestants;
	vector<string> id;
	string trstr;
	bool firstElemFlag = true;
	
	boost::property_tree::ptree propertyTree;

	//Читаем XML
	boost::property_tree::read_xml(filename, propertyTree);

	std::locale loc(russian_locale_designator);

	//--->Начало JSON-файла
	cout << "{" << endl;

	//--->contestName
	stmp = propertyTree.get<string>("contestLog.settings.contestName");
	MultiByteToWideChar(CP_UTF8, 0, stmp.c_str(), 500, wbuff, 500);
	cout << "\"contestName\": \"";
	wcout << wbuff;
	cout << "\"," << endl;


	//--->freezeTimeMinutesFromStart
	cout << "\"freezeTimeMinutesFromStart\": 240," << endl;

	//--->problemLetters
	firstElemFlag = true;
	cout << "\"problemLetters\":" << endl << "[" << endl;
	BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.problems"))
	{
		if (!firstElemFlag)
			cout << "," << endl;
		else
			firstElemFlag = false;
		cout << "\"" << v.second.get<string>("<xmlattr>.title") << "\"";
	}
	cout << endl << "]," << endl;

	//--->contestants
	firstElemFlag = true;
	cout << "\"contestants\":" << endl << "[" << endl;
	BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.users"))
	{
		try{
			v.second.get<string>("<xmlattr>.participationType");
		}
		catch (...){
			if (!firstElemFlag)
				cout << "," << endl;
			else
				firstElemFlag = false;
			stmp = v.second.get<string>("<xmlattr>.displayedName");
			MultiByteToWideChar(CP_UTF8, 0, stmp.c_str(), 500, wbuff, 500);
			cout << "\"";
			wcout << wbuff;
			cout << "\"";

			wstring ws = wbuff;
			trstr = narrow_string(ws, loc);
			contestants[v.second.get<string>("<xmlattr>.id")] = trstr;
		}
	}
	cout << endl << "]," << endl;


	/*BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.users"))
	{
		try{
			v.second.get<string>("<xmlattr>.participationType");
		}
		catch (...){
			id.push_back(v.second.get<string>("<xmlattr>.id"));
		}
	}

	int count = 0;
	BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.users"))
	{
		try{
			v.second.get<string>("<xmlattr>.participationType");
		}
		catch (...){
			stmp = v.second.get<string>("<xmlattr>.displayedName");
			contestants[id[count]] = stmp;
			++count;
		}
	}*/

	//--->runs
	firstElemFlag = true;
	cout << "\"runs\":" << endl << "[" << endl;

	BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.events"))
	{
		if (v.first == "submit"){
			string userid = v.second.get<string>("<xmlattr>.userId");
			if (contestants.count(userid) > 0){
				if (!firstElemFlag)
					cout << "," << endl;
				else
					firstElemFlag = false;
				cout << "{" << endl;
				cout << "\"contestant\": \"";
				cout << contestants[userid];
				/*MultiByteToWideChar(CP_UTF8, 0, contestants[userid].c_str(), 500, wbuff, 500);
				wcout << wbuff;*/
				cout << "\"," << endl;

				cout << "\"problemLetter\": \"";
				cout << v.second.get<string>("<xmlattr>.problemTitle");
				cout << "\"," << endl;

				cout << "\"timeMinutesFromStart\": ";
				cout << v.second.get<int>("<xmlattr>.contestTime")/60000;
				cout << "," << endl;

				cout << "\"success\": ";
				cout << v.second.get<string>("<xmlattr>.samplesPassed") << endl;

				cout << "}";
			}
		}
	}
	cout << endl << "]" << endl;

	//--->Завершение JSON-файла
	cout << "}" << endl;
	return 0;
}