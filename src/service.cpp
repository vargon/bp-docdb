/*
 *  service.cpp
 *  SearchDB
 *
 *  Created by Ashit Gandhi on 9/22/09.
 *  Copyright 2009 Yahoo! Inc.. All rights reserved.
 *
 */

#include "service.h"

SearchDBService::SearchDBService() : Service(), sa(0), writer(0)
{
}

SearchDBService::~SearchDBService()
{
	delete writer;
	delete sa;
}

void SearchDBService::init(const bplus::service::Transaction& tran, const bplus::Map& args)
{
	sa = new lucene::analysis::standard::StandardAnalyzer();
}

void SearchDBService::openIndex(const bplus::service::Transaction& tran, const bplus::Map& args)
{
	const std::string dataDir(context("data_dir"));
	
	//TODO: This is very insecure
	const std::string dbName(args["dbName"]);
	
	indexPath = (dataDir+"/"+dbName);
	
	writer = new lucene::index::IndexWriter(indexPath.c_str(), sa, true);
}

void SearchDBService::addDocument(const bplus::service::Transaction& tran, const bplus::Map& args)
{
	lucene::document::Document doc;
	
	bplus::Map::Iterator it(args);
	while(const char* key = it.nextKey())
	{
		lucene::document::Field fld(bplus::strutil::utf8ToWide(key).c_str(), bplus::strutil::utf8ToWide(std::string(args[key])).c_str(), 0);
		doc.add(fld);
	}

	writer->addDocument(&doc);
}

void SearchDBService::closeIndex(const bplus::service::Transaction& tran, const bplus::Map& args)
{
	writer->optimize();
	writer->close();
}

void SearchDBService::search(const bplus::service::Transaction& tran, const bplus::Map& args)
{
	lucene::search::IndexSearcher searcher(indexPath.c_str());
	
	std::wstring query;
	std::wstring fieldName;
	
	lucene::search::Query* q = lucene::queryParser::QueryParser::parse(query.c_str(), fieldName.c_str(), sa);
	
	lucene::search::Hits* hits = searcher.search(q);
	
	bplus::List hitList;
	for(int i = 0; i < hits->length(); ++i)
	{
		lucene::document::Document doc(hits->doc(i));

		typedef std::vector<const bplus::Object*> obvector;
		obvector fieldList = args["fields"];

		bplus::Map* map = new bplus::Map();
		for(obvector::const_iterator it = fieldList.begin(); it != fieldList.end(); ++it)
		{
			std::string key(**it);
			std::wstring wkey(bplus::strutil::utf8ToWide(key));
			
			std::wstring wvalue = doc.get(wkey.c_str());
			std::string value = bplus::strutil::wideToUtf8(wvalue);
			map->add(key, new bplus::String(value.c_str()));
		}
		hitList.append(map);
	}
	
	searcher.close();

	tran.complete( hitList );
}
