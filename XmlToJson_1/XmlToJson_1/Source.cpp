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
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace std;
using boost::property_tree::ptree;
using boost::property_tree::basic_ptree;

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
	vector<string> runtime;
	string trstr;
	ptree root;

	boost::property_tree::ptree propertyTree;
	//Читаем XML
	boost::property_tree::read_xml(filename, propertyTree);

	std::locale loc(russian_locale_designator);


	//--->contestName
	stmp = propertyTree.get<string>("contestLog.settings.contestName");
	MultiByteToWideChar(CP_UTF8, 0, stmp.c_str(), 500, wbuff, 500);
	wstring ws = wbuff;
	trstr = narrow_string(ws, loc);

	root.put<string>("contestName", trstr);

	//--->freezeTimeMinutesFromStart
	root.put<int>("freezeTimeMinutesFromStart", 240); //-int!!!!!!!!!!!!!

	//--->problemLetters
	ptree  problems;

	BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.problems"))
	{
		ptree pr;
		pr.put("", v.second.get<string>("<xmlattr>.title"));
		problems.push_back(make_pair("", pr));
	}

	root.put_child("problemLetters", problems);

	//--->contestants
	ptree  contestant;

	BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.users"))
	{
		try{
			v.second.get<string>("<xmlattr>.participationType");
		}
		catch (...){
			stmp = v.second.get<string>("<xmlattr>.displayedName");
			MultiByteToWideChar(CP_UTF8, 0, stmp.c_str(), 500, wbuff, 500);
			ws = wbuff;
			trstr = narrow_string(ws, loc);
			contestants[v.second.get<string>("<xmlattr>.id")] = trstr;

			ptree pr;
			pr.put("", trstr);
			contestant.push_back(make_pair("", pr));
		}
	}

	root.put_child("contestants", contestant);

	//--->runs
	ptree runs;

	BOOST_FOREACH(auto &v, propertyTree.get_child("contestLog.events"))
	{
		if (v.first == "submit"){
			string userid = v.second.get<string>("<xmlattr>.userId");
			if (contestants.count(userid) > 0)
			{
				ptree elem;

				elem.put<string>("contestant", contestants[userid]);
				elem.put<string>("problemLetter", v.second.get<string>("<xmlattr>.problemTitle"));
				//runtime.push_back(to_string(v.second.get<int>("<xmlattr>.contestTime") / 60000));
				elem.put<int>("timeMinutesFromStart", v.second.get<int>("<xmlattr>.contestTime") / 60000);
				elem.put<bool>("success", v.second.get<bool>("<xmlattr>.samplesPassed"));

				runs.push_back(make_pair("", elem));
			}
		}
	}

	root.put_child("runs", runs);

	string outFileName = "Output.json.txt";
	write_json(outFileName, root);

	/*ostringstream buf;
	write_json(buf, root);

	string json = buf.str();
	boost::replace_all<string>(json, "\":240\"", to_string(240));

	boost::replace_all<string>(json, "\"true\"", "true");
	boost::replace_all<string>(json, "\"false\"", "false");

	for (int i = 0; i < runtime.size(); ++i){
		if ((i + 1) % 10 == 0)
			cout << endl;
		boost::replace_all<string>(json, "\":" + runtime[i] + "\"", runtime[i]);
	}
	
	cout << json;*/


	return 0;
}