/*
 *  Settings.cpp
 *  OpenLieroX
 *
 *  Created by Albert Zeyer on 24.02.10.
 *  code under LGPL
 *
 */

#include "Settings.h"
#include "Options.h"
#include "CServer.h"
#include "IniReader.h"
#include "Debug.h"

Settings gameSettings;
FeatureSettingsLayer modSettings;
FeatureSettingsLayer gamePresetSettings;

void FeatureSettingsLayer::copyTo(FeatureSettingsLayer& s) const {
	for(size_t i = 0; i < FeatureArrayLen; ++i)
		if(isSet[i])
			s.set((FeatureIndex)i) = (*this)[(FeatureIndex)i];
}

void FeatureSettingsLayer::copyTo(FeatureSettings& s) const {
	for(size_t i = 0; i < FeatureArrayLen; ++i)
		if(isSet[i])
			s[(FeatureIndex)i] = (*this)[(FeatureIndex)i];
}

void FeatureSettingsLayer::dump() const {
	notes << "Settings layer {" << endl;
	for(size_t i = 0; i < FeatureArrayLen; ++i)
		if(isSet[i])
			notes << " " << featureArray[i].name << " : " << (*this)[(FeatureIndex)i] << endl;
	notes << "}" << endl;
}



void Settings::layersInitStandard(bool withCustom) {
	layersClear();
	layers.push_back(&modSettings);
	layers.push_back(&gamePresetSettings);
	if(withCustom) {
		if(tLXOptions)
			layers.push_back(&tLXOptions->customSettings);
		else
			errors << "Settings::layersInitStandard: tLXOptions == NULL" << endl;
	}
}


ScriptVar_t Settings::hostGet(FeatureIndex i) {
	ScriptVar_t var = (*this)[i];
	Feature* f = &featureArray[i];
	if(f->getValueFct)
		var = f->getValueFct( var );
	
	return var;
}

bool Settings::olderClientsSupportSetting(Feature* f) {
	if( f->optionalForClient ) return true;
	return hostGet(f) == f->unsetValue;
}


bool FeatureSettingsLayer::loadFromConfig(const std::string& cfgfile, bool reset, std::map<std::string, std::string>* unknown) {
	if(reset) makeSet(false);
	
	IniReader ini(cfgfile);
	if(!ini.Parse()) return false;
	
	IniReader::Section& section = ini.m_sections["GameInfo"];
	for(IniReader::Section::iterator i = section.begin(); i != section.end(); ++i) {
		Feature* f = featureByName(i->first);
		if(f) {
			FeatureIndex fi = featureArrayIndex(f);
			if(!set(fi).fromString(i->second))
				notes << "loadFromConfig " << cfgfile << ": cannot understand: " << i->first << " = " << i->second << endl;
		}
		else {
			if(unknown)
				(*unknown)[i->first] = i->second;
		}
	}
	
	return true;
}

void Settings::dumpAllLayers() const {
	notes << "Settings (" << layers.size() << " layers) {" << endl;
	size_t num = 1;
	for(Layers::const_iterator i = layers.begin(); i != layers.end(); ++i, ++num) {
		notes << "Layer " << num << " ";
		(*i)->dump();
	}
	notes << "}" << endl;
}
