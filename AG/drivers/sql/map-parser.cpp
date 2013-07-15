/* <copyright>
 * Copyright 2013 The Trustees of Princeton University
 * All Rights Reserved
 * </copyright>
 * <author>Wathsala Vithanage</author>
 * <email>wathsala@princeton.edy</email>
 * <date>06/24/2013</date>
 * <summary>
 *   map-parser.cpp: Parses an XML file in the form 
 *   <?xml version="1.0"?>
 *   <Map>
 *     <Pair>
 *       <File>/foo/bar</File>
 *       <Query>SQL</Query>
 *     </Pair>
 *   </Map>
 * </summary>
 */

#include "map-parser.h"



MapParserHandler::MapParserHandler(map<string, struct map_info>* xmlmap)
{
    this->xmlmap = xmlmap;
    open_key = false;
    open_val = false;
    is_bounded_query = true;
    element_buff = NULL;
    current_key = NULL;
    bounded_query = NULL;
    unbounded_query = NULL;
}

void MapParserHandler::startElement(const   XMLCh* const    uri,
	const   XMLCh* const    localname,
	const   XMLCh* const    qname,
	const   Attributes&     attrs)
{
    char* tag = XMLString::transcode(localname);
    if (!strncmp(tag, KEY_TAG, strlen(KEY_TAG))) {
	open_key = true;
    }
    if (!strncmp(tag, VALUE_TAG, strlen(VALUE_TAG))) {
	open_key = true;
    }
    for (XMLSize_t i=0; i < attrs.getLength(); i++) {
	char* attr = XMLString::transcode(attrs.getLocalName(i));
	if (!strncmp(attr, PERM_ATTR, strlen(PERM_ATTR))) {
	    char* perm_str = XMLString::transcode(attrs.getValue(i));
	    if (perm_str) {
		uint16_t usr = (uint16_t)perm_str[0];
		usr <<= 6;
		uint16_t grp = (uint16_t)perm_str[1];
		grp <<= 3;
		uint16_t oth = (uint16_t)perm_str[2];
		current_perm = (usr | grp | oth);
		XMLString::release(&perm_str);
	    }
	}
	if (!strncmp(attr, QUERY_BOUND_ATTR, strlen(QUERY_BOUND_ATTR))) {
	    char* bound_str = XMLString::transcode(attrs.getValue(i));
	    if (bound_str) {
		if (atol(bound_str) > 0)
		    is_bounded_query = true;
		else
		    is_bounded_query = false;
		XMLString::release(&bound_str);
	    }
	}
	XMLString::release(&attr);
    }
    XMLString::release(&tag);
}

void MapParserHandler::endElement (   
	const   XMLCh *const    uri,
	const   XMLCh *const    localname,
	const   XMLCh *const    qname) 
{
    char* tag = XMLString::transcode(localname);
    if (!strncmp(tag, KEY_TAG, strlen(KEY_TAG)) && open_key) {
	open_key = false;
	current_key = strdup(element_buff);
    }
    if (!strncmp(tag, VALUE_TAG, strlen(VALUE_TAG)) && open_key 
	    && is_bounded_query) {
	open_key = false;
	bounded_query = strdup(element_buff);
    }
    if (!strncmp(tag, VALUE_TAG, strlen(VALUE_TAG)) && open_key 
	    && !is_bounded_query) {
	open_key = false;
	is_bounded_query = true;
	unbounded_query = strdup(element_buff);
    }
    if (!strncmp(tag, PAIR_TAG, strlen(PAIR_TAG))) {
	if (current_key) {
	    struct map_info mi;
	    mi.query = NULL;
	    mi.unbounded_query = NULL;
	    mi.file_perm = current_perm;
	    if (bounded_query) {
		size_t bounded_query_len = strlen(bounded_query);
		mi.query = (unsigned char*)malloc(bounded_query_len + 1);
		strncpy((char*)mi.query, bounded_query, bounded_query_len);
		mi.query[bounded_query_len] = 0;
		free(bounded_query);
		bounded_query = NULL;
	    }
	    if (unbounded_query) {
		size_t unbounded_query_len = strlen(unbounded_query);
		mi.unbounded_query = (unsigned char*)malloc(unbounded_query_len + 1);
		strncpy((char*)mi.unbounded_query, unbounded_query, unbounded_query_len);
		mi.unbounded_query[unbounded_query_len] = 0;
		free(unbounded_query);
		unbounded_query = NULL;
	    }
	    (*xmlmap)[string(current_key)] =mi;
	    free(current_key);
	    current_key = NULL;
	    if (element_buff) {
		free(element_buff);
		element_buff = NULL;
	    }
	    XMLString::release(&tag);
	}
    }
    if (element_buff) {
	free(element_buff);
	element_buff = NULL;
    }
}

void MapParserHandler::characters (   
	const   XMLCh *const    chars,
	const   unsigned int    length)
{
    char* element = XMLString::transcode(chars);
    if (!element)
	return;
    if (open_key) {
	if (!element_buff) { 
	    element_buff = (char*)malloc(length+1);
	    element_buff[length] = 0;
	    strncpy(element_buff, element, length);
	}
	else { 
	    int current_len = strlen(element_buff); 
	    element_buff = (char*)realloc
		(element_buff, current_len + length + 1);
	    element_buff[current_len + length] = 0;
	    strncat(element_buff + current_len, element, length);
	}
	XMLString::release(&element);
    }
}

void MapParserHandler::fatalError(const SAXParseException& exception)
{
    char* message = XMLString::transcode(exception.getMessage());
    XMLString::release(&message);
}   

MapParser::MapParser( char* mapfile)
{
    this->mapfile = mapfile;
    FS2SQLMap = new map<string, struct map_info>;
}

int MapParser::parse()
{
    try {
	XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch) {
	char* message = XMLString::transcode(toCatch.getMessage());
	XMLString::release(&message);
	return 1;
    }
    SAX2XMLReader* parser = XMLReaderFactory::createXMLReader();
    parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
    parser->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);   // optional
    MapParserHandler* mph = new MapParserHandler(this->FS2SQLMap);
    parser->setContentHandler(mph);
    parser->setErrorHandler(mph);
    try {
	parser->parse(mapfile);
    }
    catch (const XMLException& toCatch) {
	char* message = XMLString::transcode(toCatch.getMessage());
	XMLString::release(&message);
	return -1;
    }
    catch (const SAXParseException& toCatch) {
	char* message = XMLString::transcode(toCatch.getMessage());
	XMLString::release(&message);
	return -1;
    }
    catch (...) {
	return -1;
    }
    delete parser;
    delete mph;
    return 0;
}
	
map<string, struct map_info>* MapParser::get_map()
{
    return FS2SQLMap;
}

