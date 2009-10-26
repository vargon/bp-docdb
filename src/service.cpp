/*
 *  service.cpp
 *  SearchDB
 *
 *  Created by Ashit Gandhi on 9/22/09.
 *  Copyright 2009 Yahoo! Inc.. All rights reserved.
 *
 */

#include "service.h"

CL_NS_USE(analysis);
CL_NS_USE2(analysis, standard);
CL_NS_USE(index);
CL_NS_USE(document);
CL_NS_USE(search);
CL_NS_USE(queryParser);

#ifdef _UNICODE
#define W2U(x)      bplus::strutil::wideToUtf8(x)
#define U2W(x)      bplus::strutil::utf8ToWide(x)
#define T2W(x)      (x)
#define T2U         W2U
#define W2T(x)      (x)
#define U2T         U2W
typedef std::wstring tstring;
#else
#define U2W(x)      bplus::strutil::utf8ToWide(x)
#define W2U(x)      bplus::strutil::wideToUtf8(x)
#define T2W         U2W
#define T2U(x)      (x)
#define W2T         W2U
#define U2T(x)      (x)
typedef std::string tstring;
#endif

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
    SCOPED_LOCK_MUTEX(myMutex);
    log(BP_INFO, "initializing search service: [" __DATE__ " " __TIME__ "]");
    log(BP_INFO, __FUNCTION__);
	sa = new StandardAnalyzer();

	tran.complete( bplus::Null() );
}

void SearchDBService::openIndex(const bplus::service::Transaction& tran, const bplus::Map& args)
{
    SCOPED_LOCK_MUTEX(myMutex);
    log(BP_INFO, __FUNCTION__);
	const std::string dataDir(context("data_dir"));
    
    if(sa) log(BP_INFO, "SA EXISTS");
    else log(BP_INFO, "SA does not exist");
	
	//TODO: This is very insecure
	const std::string dbName(args["dbName"]);
	
	indexPath = (dataDir+"/"+dbName);
	
	writer = new IndexWriter(indexPath.c_str(), sa, true);

	tran.complete( bplus::Null() );
}

void SearchDBService::addDocument(const bplus::service::Transaction& tran, const bplus::Map& args)
{
    SCOPED_LOCK_MUTEX(myMutex);
    log(BP_INFO, __FUNCTION__);
    
	Document* doc = new Document();
    
    const bplus::Map* r = NULL;
    const std::string d("doc");
    args.getMap(d, r);
    
    const bplus::Map& m = *r;
	
    log(BP_INFO, "Got map");

	bplus::Map::Iterator it(m);
    
    log(BP_INFO, "Got iterator");

	/**/
    typedef std::vector<Field*> FieldVector;
    FieldVector fieldVec;
    
    typedef std::map<TCHAR*,TCHAR*> StringMap;
    StringMap stringMap;
    
	while(const char* ckey = it.nextKey())
	{
        std::string key(ckey);
        std::string val(m[ckey]);
        log(BP_INFO, key);
        log(BP_INFO, val.c_str());

        TCHAR* tkey = new TCHAR[key.length()+1];
        TCHAR* tval = new TCHAR[val.length()+1];
        _tcscpy(tkey, U2T(key).c_str());
        _tcscpy(tval, U2T(val).c_str());
        
        log(BP_INFO, (const char*)tkey);
        log(BP_INFO, (const char*)tval);
        
        stringMap[tkey] = tval;
        log(BP_INFO, "Created map");
	}

    for(StringMap::iterator smi = stringMap.begin(); smi != stringMap.end(); ++smi)
    {
        log(BP_INFO, "Creating Fields");
        log(BP_INFO, (const char*)smi->first); // will only print first char
        log(BP_INFO, (const char*)smi->second); // will only print first char
        Field* fld = new Field(smi->first, smi->second, Field::STORE_YES|Field::INDEX_TOKENIZED);
        fieldVec.push_back(fld);
    }
    for(FieldVector::iterator fi = fieldVec.begin(); fi != fieldVec.end(); ++fi)
    {
        log(BP_INFO, "Adding fields Fields");
        doc->add(**fi);
    }
    /**/

    log(BP_INFO, "Finished creating document");
    writer->addDocument(doc);
    log(BP_INFO, "Added document");

	for(FieldVector::iterator fi = fieldVec.begin(); fi != fieldVec.end(); ++fi)
	{
		delete (*fi);
	}

	for(StringMap::iterator smi = stringMap.begin(); smi != stringMap.end(); ++smi)
	{
		delete [] (smi->first);
		delete [] (smi->second);
	}
    
	tran.complete( bplus::Null() );
}

void SearchDBService::closeIndex(const bplus::service::Transaction& tran, const bplus::Map& args)
{
    SCOPED_LOCK_MUTEX(myMutex);
    log(BP_INFO, __FUNCTION__);
	writer->optimize();
    log(BP_INFO, "Optimized");
	writer->close();
    log(BP_INFO, "Closed");
	tran.complete( bplus::Null() );
}

void SearchDBService::search(const bplus::service::Transaction& tran, const bplus::Map& args)
{
    SCOPED_LOCK_MUTEX(myMutex);
    log(BP_INFO, __FUNCTION__);
    log(BP_INFO, "search() invoked 3: " + indexPath);
    
	lucene::search::IndexSearcher searcher(indexPath.c_str());
	
	tstring query(U2T(args["query"]));

    typedef std::vector<const bplus::Object*> obvector;
    obvector fieldList = args["fields"];

    tstring fieldName(U2T(std::string(**(fieldList.begin()))));
	
    log(BP_INFO, ("about to query " + T2U(query) + ", " + T2U(fieldName)));
    log(BP_INFO, (T2U(query).c_str()));
    log(BP_INFO, (T2U(fieldName).c_str()));

	lucene::search::Query* q = lucene::queryParser::QueryParser::parse(query.c_str(), fieldName.c_str(), sa);

    if(q)
        log(BP_INFO, "Set up the query");
    else {
        log(BP_INFO, "Failed to set up the query");
    }


	lucene::search::Hits* hits = searcher.search(q);
	
    log(BP_INFO, "did the query");
	bplus::List hitList;
	for(int i = 0; i < hits->length(); ++i)
	{
        log(BP_INFO, "Checking document");
		lucene::document::Document doc(hits->doc(i));

		bplus::Map* map = new bplus::Map();
		for(obvector::const_iterator it = fieldList.begin(); it != fieldList.end(); ++it)
		{
			std::string key(**it);
			tstring tkey(U2T(key));
			
			tstring value(doc.get(tkey.c_str()));
            log(BP_INFO, (const char*)(value.c_str()));
			//std::string value(bplus::strutil::wideToUtf8(wvalue));
			map->add(key, new bplus::String(T2U(value).c_str()));
		}
		hitList.append(map);
	}
	
	searcher.close();

	tran.complete( hitList );
    log(BP_INFO, "search() returned hits");
}
