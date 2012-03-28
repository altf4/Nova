//============================================================================
// Name        : HoneydConfiguration.h
// Copyright   : DataSoft Corporation 2011-2012
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Object for reading and writing Honeyd XML configurations
//============================================================================/*

#ifndef _HONEYDCONFIGURATION
#define _HONEYDCONFIGURATION

#include "Config.h"
#include "NovaGuiTypes.h"
#include "Defines.h"

class HoneydConfiguration
{
public:
    HoneydConfiguration();

    //XML Read Functions
    //calls main load functions
    void LoadAllTemplates();
    //load all scripts
    void LoadScriptsTemplate();
    //load all ports
    void LoadPortsTemplate();
    //load all profiles
    void LoadProfilesTemplate();
    //load current honeyd configuration group
    void LoadNodesTemplate();

    //set profile configurations
    void LoadProfileSettings(boost::property_tree::ptree *ptr, profile *p);
    //add ports or subsystems
    void LoadProfileServices(boost::property_tree::ptree *ptr, profile *p);
    //recursive descent down profile tree
    void LoadProfileChildren(std::string parent);

    //Outputs the profile in a string formatted for direct insertion to a honeyd configuration file.
    std::string ProfileToString(profile* p);

    //Outputs the profile in a string formatted for direct insertion to a honeyd configuration file.
    // This function differs from ProfileToString in that it omits values incompatible with the loopback interface
    std::string DoppProfileToString(profile* p);


    //Load stored subnets in ptr
    void LoadSubnets(boost::property_tree::ptree *ptr);
    //Load stored honeyd nodes ptr
    void LoadNodes(boost::property_tree::ptree *ptr);

    //Saves the current configuration information to XML files
    void SaveAllTemplates();
    //Writes the current configuration to honeyd configs
    void WriteHoneydConfiguration(std::string path);

    SubnetTable GetSubnets() const;
    PortTable GetPorts() const;
    NodeTable GetNodes() const;
    ScriptTable GetScripts() const;
    ProfileTable GetProfiles() const;

    void SetSubnets(SubnetTable subnets);
    void SetPorts(PortTable ports);
    void SetNodes(NodeTable nodes);
    void SetScripts(ScriptTable scripts);
    void SetProfiles(ProfileTable profile);

    // Returns the number of bits used in the mask when given in in_addr_t form
    static int GetMaskBits(in_addr_t mask);


private:
    std::string m_homePath;

    //Storing these trees allow for easy modification and writing of the XML files
    //Without having to reconstruct the tree from scratch.
    boost::property_tree::ptree m_groupTree;
    boost::property_tree::ptree m_portTree;
    boost::property_tree::ptree m_profileTree;
    boost::property_tree::ptree m_scriptTree;
    boost::property_tree::ptree m_nodesTree;
    boost::property_tree::ptree m_subnetTree;

    SubnetTable m_subnets;
    PortTable m_ports;
    ProfileTable m_profiles;
    NodeTable m_nodes;
    ScriptTable m_scripts;
};

#endif
