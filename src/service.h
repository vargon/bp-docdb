/*
 *  service.h
 *  SearchDB
 *
 *  Created by Ashit Gandhi on 9/22/09.
 *  Copyright 2009 Yahoo! Inc.. All rights reserved.
 *
 */

//#if defined(MACOSX)
//#include <Carbon/Carbon.h>
//#endif

//#ifdef __OBJC__
//#import <Cocoa/Cocoa.h>
//#endif

#include "bpservice/bpservice.h"
#include "bputil/bpstrutil.h"

#if defined(MACOSX)
#define _ASCII
#endif

#include "CLucene.h"

class SearchDBService : public bplus::service::Service
{
public:
	BP_SERVICE(SearchDBService);
	
	SearchDBService();
	~SearchDBService();
	
	void init(const bplus::service::Transaction& tran, const bplus::Map& args);
	void init2(const bplus::service::Transaction& tran, const bplus::Map& args);
	void openIndex(const bplus::service::Transaction& tran, const bplus::Map& args);
	void addDocument(const bplus::service::Transaction& tran, const bplus::Map& args);
	void closeIndex(const bplus::service::Transaction& tran, const bplus::Map& args);
	void search(const bplus::service::Transaction& tran, const bplus::Map& args);
	
private:
	lucene::analysis::standard::StandardAnalyzer* sa;
	lucene::index::IndexWriter* writer;
	std::string indexPath;
    DEFINE_MUTEX(myMutex);
};

BP_SERVICE_DESC(SearchDBService, "SearchDBService", "1.0.0", "A searchable database based on Lucene")

ADD_BP_METHOD(SearchDBService, init, "Initializes the Searchable Database")

ADD_BP_METHOD(SearchDBService, openIndex, "Opens the index for writing")
	ADD_BP_METHOD_ARG(openIndex, "dbName", String, true, "The name of the database to create or open")

ADD_BP_METHOD(SearchDBService, addDocument, "Adds a document to the index")
	ADD_BP_METHOD_ARG(addDocument, "doc", Map, true, "A map representing the document")

ADD_BP_METHOD(SearchDBService, closeIndex, "Closes and optimizes the index")

ADD_BP_METHOD(SearchDBService, search, "Searches the index and returns all matches")
	ADD_BP_METHOD_ARG(search, "query", String, true, "A query string to search for")
	ADD_BP_METHOD_ARG(search, "fields", List, true, "A list of fields to return values for")

END_BP_SERVICE_DESC
