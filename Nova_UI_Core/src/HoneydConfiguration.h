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
#include "VendorMacDb.h"

namespace Nova
{

typedef std::string profileName;

enum hdConfigReturn
{
	INHERITED,
	NOT_INHERITED,
	NO_SUCH_KEY
};


class HoneydConfiguration
{
public:
    HoneydConfiguration();

    //XML Read Functions
    //calls main load functions
    bool LoadAllTemplates();


    //Outputs the profile in a string formatted for direct insertion to a honeyd configuration file.
    std::string ProfileToString(profile* p);

    //Outputs the profile in a string formatted for direct insertion to a honeyd configuration file.
    // This function differs from ProfileToString in that it omits values incompatible with the loopback interface
    std::string DoppProfileToString(profile* p);

    //Saves the current configuration information to XML files
    void SaveAllTemplates();
    //Writes the current configuration to honeyd configs
    void WriteHoneydConfiguration(std::string path);

    //Setter for the directory to read from and write to
    void SetHomePath(std::string homePath);
    //Getter for the directory to read from and write to
    std::string GetHomePath();

    // Returns the number of bits used in the mask when given in in_addr_t form
    static int GetMaskBits(in_addr_t mask);


    // Some high level node creation methods

    // Add a node with static IP and static MAC
    bool AddNewNode(std::string profile, std::string ipAddress, std::string macAddress, std::string interface, std::string subnet);
    bool AddNewNodes(std::string profile, std::string ipAddress,std::string interface, std::string subnet, int numberOfNodes);


	std::vector<std::string> GetProfileChildren(std::string parent);

	std::vector<std::string> GetProfileNames();
	Nova::profile * GetProfile(std::string name);
	Nova::port * GetPort(std::string name);

	std::vector<std::string> GetNodeNames();
	std::vector<std::string> GetSubnetNames();


	// These were an attempt to make things easier to bind with javascript. Didn't work well,
	// right now these are unused and can probably be thrown out.
	std::pair <hdConfigReturn, std::string> GetEthernet(profileName profile);
	std::pair <hdConfigReturn, std::string> GetPersonality(profileName profile);
	std::pair <hdConfigReturn, std::string> GetDroprate(profileName profile);
	std::pair <hdConfigReturn, std::string> GetActionTCP(profileName profile);
	std::pair <hdConfigReturn, std::string> GetActionUDP(profileName profile);
	std::pair <hdConfigReturn, std::string> GetActionICMP(profileName profile);
	std::pair <hdConfigReturn, std::string> GetUptimeMin(profileName profile);
	std::pair <hdConfigReturn, std::string> GetUptimeMax(profileName profile);

	// Returns list of all available scripts
	std::vector<std::string> GetScriptNames();

	bool IsMACUsed(std::string mac);
	bool IsIPUsed(std::string ip);
	bool IsProfileUsed(std::string profile);

	// Regenerates the MAC addresses for nodes of this profile
	void RegenerateMACAddresses(std::string profileName);
	std::string GenerateUniqueMACAddress(std::string vendor);

    void DeleteProfile(profile *p);
    bool DeleteProfile(std::string profileName);

    void RenameProfile(profile *p, std::string newName);

    //Deletes a single node, called from deleteNodes();
    bool DeleteNode(std::string node);
    Node * GetNode(std::string name);

    std::string GetNodeSubnet(std::string node);
    bool EnableNode(std::string node);
    bool DisableNode(std::string node);
    void DisableProfileNodes(std::string profile);



// TODO: this should be private eventually
public:
	SubnetTable m_subnets;
	PortTable m_ports;
	ProfileTable m_profiles;
    NodeTable m_nodes;

private:
    std::string m_homePath;


    VendorMacDb m_macAddresses;

    //Storing these trees allow for easy modification and writing of the XML files
    //Without having to reconstruct the tree from scratch.
    boost::property_tree::ptree m_groupTree;
    boost::property_tree::ptree m_portTree;
    boost::property_tree::ptree m_profileTree;
    boost::property_tree::ptree m_scriptTree;
    boost::property_tree::ptree m_nodesTree;
    boost::property_tree::ptree m_subnetTree;

    ScriptTable m_scripts;

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

    //Load stored subnets in ptr
    void LoadSubnets(boost::property_tree::ptree *ptr);
    //Load stored honeyd nodes ptr
    void LoadNodes(boost::property_tree::ptree *ptr);

    std::string FindSubnet(in_addr_t ip);
};

}

#endif
