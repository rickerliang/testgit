from xml.sax import *
from xml.dom import minidom 
import io

class TrieNode(object):  
    def __init__(self):  
        self.value = None  
        self.children = {}  
  
class Trie(object):  
    def __init__(self):  
        self.root = TrieNode()  
  
    def add(self, key):  
        node = self.root  
        for char in key:  
            if char not in node.children:  
                child = TrieNode()  
                node.children[char] = child  
                node = child  
            else:  
                node = node.children[char]  
        node.value = key  
  
    def search(self, key): 
        node = self.root  
        matches = []  
        for char in key:  
            if char not in node.children:  
                return None  
            node = node.children[char]   
        return node.value

class KuipSaxHandler(handler.ContentHandler):
    def __init__(self):
        self.resultList = []
    def startDocument(self):
        pass
    def endDocument(self):
        pass
    def startElement(self,name,attrs):
        try:
            self.resultList.append(attrs.getValue('name').strip())
        except KeyError:
            pass
    def endElement(self,name):
        pass
    def characters(self,ch):
        pass

class TSSaxHandler(handler.ContentHandler):
    def __init__(self):
        self.result = Trie()
        self.currentNode = ''
    def startDocument(self):
        pass
    def endDocument(self):
        pass
    def startElement(self,name,attrs):
        if name == 'source':
            self.currentNode = 'source'
        else:
            self.currentNode = ''
    def endElement(self,name):
        pass
    def characters(self,ch):
        if self.currentNode == 'source':
            if self.result.search(ch.strip()) == None:
                self.result.add(ch.strip())

def parse_kuip(intput_data, handler):
    parser = make_parser()
    parser.setContentHandler(handler)    
    parser.parse(io.StringIO(data))

def insert_translation_message(message, parentNode, doc):
    messageNode = doc.createElement('message')
    sourceNode = doc.createElement('source')
    sourceText = doc.createTextNode(message)
    sourceNode.appendChild(sourceText)
    messageNode.appendChild(sourceNode)
    parentNode.appendChild(messageNode)
    translationStateNode = doc.createElement('translation')
    translationStateNode.setAttribute('type', 'unfinished')
    messageNode.appendChild(translationStateNode)

def output_ts(unique_names_list, exist_names):
    f = open("c:\\SkInternalTranslator.ts", mode='r', encoding='utf-8')
    doc = None
    try:
        doc = minidom.parse(f)
    except:
        doc = minidom.Document()
    f.close()
    rootNode = doc.documentElement
    if rootNode == None:
        rootNode = doc.createElement('TS')
        rootNode.setAttribute('version', '2.0')
        rootNode.setAttribute('language', 'zh_CN')
        doc.appendChild(rootNode)
    else:
        pass

    contextNodeList = rootNode.getElementsByTagName('context')
    if contextNodeList.length == 0:
        contextNode = doc.createElement('context')
        rootNode.appendChild(contextNode)
        contextName = doc.createElement('name')
        contextContent = doc.createTextNode('SkInternalTranslator')
        contextName.appendChild(contextContent)
        contextNode.appendChild(contextName)    
    else:
        contextNode = contextNodeList.item(0)        
    
    newCount = 0
    for line in unique_names_list:
        line = line.strip()
        if exist_names.search(line) == None:
            insert_translation_message(line, contextNode, doc)
            newCount = newCount + 1
    f = open("c:\\SkInternalTranslator.ts", mode='w', encoding='utf-8')
    doc.writexml(f, "", "", "", "utf-8")
    f.close()
    print('add new', newCount)

def scan_ts(scanHandler):
    data = ''
    try:
        xml_file = open("c:\\SkInternalTranslator.ts", mode='r', encoding='utf-8')
        data = xml_file.read().strip()
        parser = make_parser()
        parser.setContentHandler(scanHandler)
        try:
            parser.parse(io.StringIO(data))
        except:
            pass
    except:
        pass

if(__name__=="__main__"):
    data=""
    handler = KuipSaxHandler()
    print('begin parse')
    with open("c:\\common.kuip", mode='r', encoding='utf-8') as xml_file:
        data = xml_file.read().strip()
        parse_kuip(data, handler)
    with open("c:\\wpp.kuip", mode='r', encoding='utf-8') as xml_file:
        data = xml_file.read().strip()
        parse_kuip(data, handler)
    with open("c:\\wps.kuip", mode='r', encoding='utf-8') as xml_file:
        data = xml_file.read().strip()
        parse_kuip(data, handler)
    with open("c:\\et.kuip", mode='r', encoding='utf-8') as xml_file:
        data = xml_file.read().strip()
        parse_kuip(data, handler)
    print('done')
    print('begin unique')
    trie = Trie()
    uniqueList = []
    for line in handler.resultList:
        line = line.strip()
        if trie.search(line) == None:
            trie.add(line)
            uniqueList.append(line)
    print('done')
    scanHandler = TSSaxHandler()
    scan_ts(scanHandler)
    print('output ts')
    output_ts(uniqueList, scanHandler.result)
    print('done')
    
