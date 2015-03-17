//
//  mimarksattributescommand.cpp
//  Mothur
//
//  Created by Sarah Westcott on 3/17/15.
//  Copyright (c) 2015 Schloss Lab. All rights reserved.
//

#include "mimarksattributescommand.h"

//**********************************************************************************************************************
vector<string> MimarksAttributesCommand::setParameters(){
    try {
        CommandParameter pxml("xml", "InputTypes", "", "", "none", "none", "none","summary",false,false,true); parameters.push_back(pxml);
        CommandParameter psets("package", "String", "", "", "", "", "","",false,false); parameters.push_back(psets);
        CommandParameter pinputdir("inputdir", "String", "", "", "", "", "","",false,false); parameters.push_back(pinputdir);
        CommandParameter poutputdir("outputdir", "String", "", "", "", "", "","",false,false); parameters.push_back(poutputdir);
        
        vector<string> myArray;
        for (int i = 0; i < parameters.size(); i++) {	myArray.push_back(parameters[i].name);		}
        return myArray;
    }
    catch(exception& e) {
        m->errorOut(e, "MimarksAttributesCommand", "setParameters");
        exit(1);
    }
}
//**********************************************************************************************************************
string MimarksAttributesCommand::getOutputPattern(string type) {
    try {
        string pattern = "";
        
        if (type == "source") {  pattern = "[filename],source"; }
        else { m->mothurOut("[ERROR]: No definition for type " + type + " output pattern.\n"); m->control_pressed = true;  }
        
        return pattern;
    }
    catch(exception& e) {
        m->errorOut(e, "MimarksAttributesCommand", "getOutputPattern");
        exit(1);
    }
}
//**********************************************************************************************************************
string MimarksAttributesCommand::getHelpString(){
    try {
        string helpString = "";
        helpString += "Reads bioSample Attributes xml and generates source for get.mimarkspackage command. Only parameter required is xml.\n";
        helpString += "The package parameter allows you to set the package you want. Default MIMARKS.survey.\n";
        return helpString;
    }
    catch(exception& e) {
        m->errorOut(e, "MimarksAttributesCommand", "getHelpString");
        exit(1);
    }
}
//**********************************************************************************************************************
MimarksAttributesCommand::MimarksAttributesCommand(){
    try {
        abort = true; calledHelp = true;
        setParameters();
        vector<string> tempOutNames;
        outputTypes["source"] = tempOutNames;
    }
    catch(exception& e) {
        m->errorOut(e, "MimarksAttributesCommand", "MimarksAttributesCommand");
        exit(1);
    }
}
//**********************************************************************************************************************
MimarksAttributesCommand::MimarksAttributesCommand(string option)  {
    try {
        abort = false; calledHelp = false;
        
        //allow user to run help
        if(option == "help") { help(); abort = true; calledHelp = true; }
        else if(option == "citation") { citation(); abort = true; calledHelp = true;}
        
        else {
            vector<string> myArray = setParameters();
            
            OptionParser parser(option);
            map<string,string> parameters = parser.getParameters();
            
            ValidParameters validParameter;
            map<string,string>::iterator it;
            
            //check to make sure all parameters are valid for command
            for (it = parameters.begin(); it != parameters.end(); it++) {
                if (validParameter.isValidParameter(it->first, myArray, it->second) != true) {  abort = true;  }
            }
            
            //if the user changes the input directory command factory will send this info to us in the output parameter
            string inputDir = validParameter.validFile(parameters, "inputdir", false);
            if (inputDir == "not found"){	inputDir = "";		}
            else {
                string path;
                it = parameters.find("xml");
                //user has given a template file
                if(it != parameters.end()){
                    path = m->hasPath(it->second);
                    //if the user has not given a path then, add inputdir. else leave path alone.
                    if (path == "") {	parameters["xml"] = inputDir + it->second;		}
                }
            }
            
            vector<string> tempOutNames;
            outputTypes["source"] = tempOutNames;
            
            //check for required parameters
            xmlFile = validParameter.validFile(parameters, "xml", true);
            if (xmlFile == "not open") { abort = true; }
            else if (xmlFile == "not found") {  xmlFile = ""; abort=true; m->mothurOut("You must provide an xml file. It is required."); m->mothurOutEndLine();  }
            
            selectedPackage = validParameter.validFile(parameters, "package", false);
            if (selectedPackage == "not found") { selectedPackage = "MIMARKS.survey"; }
            
            //if the user changes the output directory command factory will send this info to us in the output parameter
            outputDir = validParameter.validFile(parameters, "outputdir", false);		if (outputDir == "not found"){	outputDir = m->hasPath(xmlFile);		}
            
        }
        
    }
    catch(exception& e) {
        m->errorOut(e, "MimarksAttributesCommand", "MimarksAttributesCommand");
        exit(1);
    }
}
//**********************************************************************************************************************

int MimarksAttributesCommand::execute(){
    try {
        
        if (abort == true) { if (calledHelp) { return 0; }  return 2;	}
        
        ifstream in;
        m->openInputFile(xmlFile, in);
        string header = m->getline(in); m->gobble(in);
        
        if (header != "<BioSampleAttributes>") { m->mothurOut("[ERROR]: " + header + " is not a bioSample attribute file.\n"); m->control_pressed = true; }
        
        map<string, Group> categories;
        map<string, Group>::iterator it;
        
        while (!in.eof()) {
            if (m->control_pressed) { in.close(); return 0; }
            
            Attribute attribute = readAttribute(in);
            
            if (attribute.name != "") {
                if (m->debug) {
                    m->mothurOut("[DEBUG]: name=" + attribute.name + " harmonizedName=" + attribute.harmonizedName + " format=" + attribute.format + " description=" + attribute.description + " package=" + attribute.getPackagesString() + "\n");
                }
                
                for (int i = 0; i < attribute.packages.size(); i++) {
                    it = categories.find(attribute.packages[i].groupName);
                    if (it != categories.end()) { //we already have this category, ie air, soil...
                        if (attribute.packages[i].name == (it->second).packageName) { //add attribute to category
                            (it->second).values[attribute.harmonizedName] = attribute.packages[i].required;
                        }
                    }else {
                        if ((attribute.packages[i].groupName == "\"Built\"")) {}
                        else {
                            Group thisGroup(attribute.packages[i].name);
                            thisGroup.values[attribute.harmonizedName] = attribute.packages[i].required;
                            categories[attribute.packages[i].groupName] = thisGroup;
                        }
                    }
                }
            }
        }
        in.close();
        
        string requiredByALL = "*sample_name    *description	*sample_title";
        string environment = "\"Environment\"";
        it = categories.find(environment);
        if (it != categories.end()) {
            map<string, bool>::iterator itValue = (it->second).values.begin();
            if (itValue->second) { requiredByALL += "\t*" + itValue->first; }
            itValue++;
            for (; itValue != (it->second).values.end(); itValue++) {
                if (itValue->second) { requiredByALL += "\t*" + itValue->first; }
            }
        }
        
        ofstream out;
        map<string, string> variables;
        variables["[filename]"] = outputDir + m->getRootName(m->getSimpleName(xmlFile));
        string outputFileName = getOutputFileName("source",variables);
        outputNames.push_back(outputFileName); outputTypes["source"].push_back(outputFileName);
        m->openOutputFile(outputFileName, out);
        
        for (it = categories.begin(); it != categories.end(); it++) {
            if ((it->first  == "\"Environment\"")) {}
            else {
                //create outputs
                string requiredValues = requiredByALL; string nonRequiredValues = "";
                map<string, bool>::iterator itValue = (it->second).values.begin();
                if (itValue->second) { requiredValues += "\t*" + itValue->first; }
                else { nonRequiredValues += itValue->first; }
                itValue++;
                for (; itValue != (it->second).values.end(); itValue++) {
                    if (itValue->second) { requiredValues += "\t*" + itValue->first; }
                    else { nonRequiredValues += "\t" + itValue->first; }
                }
                
                out << "else if (package == " + it->first + ") {\n";
                out << "\tout << \"#" + it->second.packageName + "\" << endl;\n";
                out << "\t if (requiredonly) {\n";
                out << "\t\tout << \"" + requiredValues + "\" << endl;\n";
                out << "}else {\n";
                out << "\t\tout << \"" + requiredValues + '\t' + nonRequiredValues + "\" << endl;\n";
                out << "\t}\n";
                out << "}";
            }
        }
        
        out.close();
        
        m->mothurOutEndLine();
        m->mothurOut("Output File Names: "); m->mothurOutEndLine();
        for (int i = 0; i < outputNames.size(); i++) {	m->mothurOut(outputNames[i]); m->mothurOutEndLine();	}	
        m->mothurOutEndLine();
        
        return 0;		
    }
    
    catch(exception& e) {
        m->errorOut(e, "MimarksAttributesCommand", "execute");
        exit(1);
    }
}
//**********************************************************************************************************************

Attribute MimarksAttributesCommand::readAttribute(ifstream& in){
    try {
        
        
        //read <Attribute>
        string header = m->getline(in); m->gobble(in);
        
        if (header == "</BioSampleAttributes>") { Attribute temp; return temp; }
        
        if (header != "<Attribute>") {  m->mothurOut("[ERROR]: " + header + ", expected '<Attribute>' in file.\n"); m->control_pressed = true;  }
        
        //read name
        //<Name>wastewater type</Name>
        m->gobble(in);
        string name = m->getline(in); m->gobble(in);
        trimTags(name);
        
        //read hamonized name
        //<HarmonizedName>wastewater_type</HarmonizedName>
        m->gobble(in);
        string hname = m->getline(in); m->gobble(in);
        trimTags(hname);
        
        //read description
        //<Description>
        //the origin of wastewater such as human waste, rainfall, storm drains, etc.
        //</Description>
        string description = "";
        unsigned long long spot = in.tellg();
        m->gobble(in);
        char c = in.get(); c = in.get();
        if (c == 'D') { //description
            description += "<D";
            while (!in.eof()) {
                m->gobble(in);
                string thisLine = m->getline(in); m->gobble(in);
                description += thisLine;
                if (thisLine.find("</Description>") != string::npos)  { break; }
            }
            trimTags(description);
        }else { //package
            in.seekg(spot);
        }
        
        //read format
        //<Format>{text}</Format>
        spot = in.tellg();
        m->gobble(in);
        c = in.get(); c = in.get();
        string format = "";
        if (c == 'F') { //format
            format += "<F" + m->getline(in); m->gobble(in);
            if (format.find("</Format>") == string::npos) { //format is not on oneline
                while (!in.eof()) {
                    m->gobble(in);
                    string thisLine = m->getline(in); m->gobble(in);
                    format += thisLine;
                    if (thisLine.find("</Format>") != string::npos) { break; }
                }
            }
            trimTags(format);
        }else { //package
            in.seekg(spot);
        }
        
        Attribute attribute(hname, description, name, format);
        
        //read Synonym - may be none
        //<Synonym>ref biomaterial</Synonym>
        bool FirstTime = true;
        while (!in.eof()) {
            unsigned long long thisspot = in.tellg();
            m->gobble(in);
            char c = in.get(); c = in.get();
            if (c == 'S') { //synonym
                FirstTime = false;
                m->getline(in); m->gobble(in);
            }else { //package
                if (FirstTime) { in.seekg(spot); }
                else { in.seekg(thisspot); }
                break;
            }
        }

        
        //read packages - may be none
        //<Package use="optional" group_name="Air">MIGS.ba.air.4.0</Package>
        while (!in.eof()) {
            string package = m->getline(in); m->gobble(in);
            if (package == "</Attribute>") { break; }
            else {
                Package thisPackage = parsePackage(package);
                if (thisPackage.groupName != "ignore") { attribute.packages.push_back(thisPackage); }
            }
        }
        
        return attribute;
    }
    
    catch(exception& e) {
        m->errorOut(e, "MimarksAttributesCommand", "execute");
        exit(1);
    }
}
//**********************************************************************************************************************

Package MimarksAttributesCommand::parsePackage(string package){
    try {
        string openingTag = trimTags(package);
        Package thispackage; thispackage.name = package;
        
        //only care about packages from our selection
        if (thispackage.name.find(selectedPackage) == string::npos) {  thispackage.groupName = "ignore"; return thispackage; }
        
        int pos = openingTag.find("use");
        if (pos != string::npos) {
            //read required or not
            string use = openingTag.substr(openingTag.find_first_of("\""), 11);
            if (use == "\"mandatory\"") { thispackage.required = true; }
        }else {
            m->mothurOut("[ERROR]: parsing error - " + openingTag + ". Expeacted something like <Package use=\"optional\" group_name=\"Air\"> in file.\n"); m->control_pressed = true;  return thispackage;
        }
        
        pos = openingTag.find("group_name");
        if (pos != string::npos) {
            //read groupname
            string group = openingTag.substr(pos);
            group = group.substr(group.find_first_of("\""), (group.find_last_of("\"")-group.find_first_of("\""))+1);
            thispackage.groupName = group;
        }else {
             thispackage.groupName = "ignore";
        }
        
        return thispackage;
    }
    
    catch(exception& e) {
        m->errorOut(e, "MimarksAttributesCommand", "parsePackage");
        exit(1);
    }
}
//**********************************************************************************************************************

string MimarksAttributesCommand::trimTags(string& value){
    try {
        string forwardTag = "";
        string thisValue = "";
        int openCarrot = 0;
        int closedCarrot = 0;
        
        
        for (int i = 0; i < value.length(); i++) {
            if (m->control_pressed) { return forwardTag; }
            
            if (value[i] == '<')         { openCarrot++;     }
            else if (value[i] == '>')    { closedCarrot++;   }
            
            //you are reading front tag
            if ((openCarrot == 1) && (closedCarrot == 0)) { forwardTag += value[i];  }
            
            if (openCarrot == closedCarrot) { //reading value
                if (value[i] != '>') { thisValue += value[i]; }
            }
            
            if (openCarrot > 1) { break; }
        }
    
        value = thisValue;
        return (forwardTag + '>');
    }
    
    catch(exception& e) {
        m->errorOut(e, "MimarksAttributesCommand", "trimTags");
        exit(1);
    }
}
//**********************************************************************************************************************


