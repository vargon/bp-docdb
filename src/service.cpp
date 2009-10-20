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
    
    /**/
    StandardAnalyzer s;
	const std::string dataDir(context("data_dir"));
    const std::string indexPath(dataDir+"/mytest");
    IndexWriter iw(indexPath.c_str(), sa, true);
    
    std::wstring k(L"body");
    std::wstring v(L"test value for the missing body");
    Document d;
    Field* f = new Field(k.c_str(), v.c_str(), Field::STORE_YES|Field::INDEX_TOKENIZED);
    d.add(*f);
    
    iw.addDocument(&d);
    iw.optimize();
    iw.close();
    
    IndexSearcher is(indexPath.c_str());
    
    Query* q = QueryParser::parse(L"value", L"body", &s);
    
    if(q)
    {
        log(BP_INFO, "q has a value!");
    }

	lucene::search::Hits* hits = is.search(q);
	
    log(BP_INFO, "did the query");
	for(int i = 0; i < hits->length(); ++i)
	{
        log(BP_INFO, "Checking document");

        std::wstring val(hits->doc(i).get(L"body"));
        
        log(BP_INFO, "Got value: " + bplus::strutil::wideToUtf8(val));
	}
	
	is.close();
    /**/
    
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
	
//	writer = new IndexWriter(indexPath.c_str(), sa, true);

	tran.complete( bplus::Null() );
}

void SearchDBService::addDocument(const bplus::service::Transaction& tran, const bplus::Map& args)
{
    SCOPED_LOCK_MUTEX(myMutex);
    log(BP_INFO, __FUNCTION__);
    
    //IndexWriter iw(indexPath.c_str(), sa, true);
    
	Document* doc = new Document();
    
    const bplus::Map* r = NULL;
    const std::string d("doc");
    args.getMap(d, r);
    
    const bplus::Map& m = *r;
	
    log(BP_INFO, "Got map");

	bplus::Map::Iterator it(m);
    
    log(BP_INFO, "Got iterator");
    
    Field* f = new Field(L"body", L"You have no idea what you're missing. Potentially.", Field::STORE_YES|Field::INDEX_TOKENIZED);
    
    doc->add(*f);
    /*
    typedef std::vector<Field*> FieldVector;
    FieldVector fieldVec;
    
    typedef std::map<TCHAR*,TCHAR*> StringMap;
    StringMap stringMap;
    
	while(const char* key = it.nextKey())
	{
        std::string val(m[key]);
        log(BP_INFO, key);
        log(BP_INFO, val.c_str());
        std::wstring wkey(bplus::strutil::utf8ToWide(key));
        std::wstring wval(bplus::strutil::utf8ToWide(val));
        TCHAR* tkey = new TCHAR[wkey.length()+1];
        TCHAR* tval = new TCHAR[wval.length()+1];
        //wcscpy(tkey, wkey.c_str());
        //wcscpy(tval, wval.c_str());
        memcpy(tkey, wkey.c_str(), (wkey.length()+1)*sizeof(TCHAR));
        memcpy(tval, wval.c_str(), (wval.length()+1)*sizeof(TCHAR));
        log(BP_INFO, (const char*)tkey);
        log(BP_INFO, (const char*)tval);
        stringMap[tkey] = tval;
        log(BP_INFO, "Created map");
	}

    for(StringMap::iterator smi = stringMap.begin(); smi != stringMap.end(); ++smi)
    {
        log(BP_INFO, "Creating Fields");
        log(BP_INFO, "Foo: " + bplus::strutil::wideToUtf8(smi->first));
        log(BP_INFO, (const char*)smi->first);
        log(BP_INFO, (const char*)smi->second);
        Field* fld = new Field(smi->first, smi->second, Field::STORE_YES|Field::INDEX_TOKENIZED);
        fieldVec.push_back(fld);
    }
    for(FieldVector::iterator fi = fieldVec.begin(); fi != fieldVec.end(); ++fi)
    {
        log(BP_INFO, "Adding fields Fields");
        doc->add(**fi);
    }
    */

    log(BP_INFO, "Finished creating document");
	//iw.addDocument(doc);
    writer->addDocument(doc);
    log(BP_INFO, "Added document");
    
    //iw.optimize();
    //iw.close();

    //writer->optimize();
    //log(BP_INFO, "Optimized document");

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
	
    std::string query(args["query"]);
	std::wstring wquery(bplus::strutil::utf8ToWide(query));

    typedef std::vector<const bplus::Object*> obvector;
    obvector fieldList = args["fields"];

    std::string fieldName(std::string(**(fieldList.begin())));
    std::wstring wfieldName(bplus::strutil::utf8ToWide(fieldName));
	
    log(BP_INFO, ("about to query " + query + ", " + fieldName));
    //lucene::queryParser::QueryParser qp(wfieldName.c_str(), sa);
    //lucene::search::Query* q = qp.parse(wquery.c_str());
	//lucene::search::Query* q = lucene::queryParser::QueryParser::parse(wquery.c_str(), wfieldName.c_str(), sa);
	lucene::search::Query* q = lucene::queryParser::QueryParser::parse(L"missing", L"body", sa);

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
			std::wstring wkey(bplus::strutil::utf8ToWide(key));
			
			std::wstring wvalue = doc.get(wkey.c_str());
            log(BP_INFO, (const char*)(wvalue.c_str()));
			std::string value = bplus::strutil::wideToUtf8(wvalue);
			map->add(key, new bplus::String(value.c_str()));
		}
		hitList.append(map);
	}
	
	searcher.close();

	tran.complete( hitList );
    log(BP_INFO, "search() returned hits");
}
