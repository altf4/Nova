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

enum portBehavior
{
	BLOCK = 0,
	RESET,
	OPEN,
	SCRIPT
};
enum recursiveDirection
{
	ALL = 0,
	UP,
	DOWN
};

enum portProtocol : bool
{
	UDP = 0,
	TCP
};

class HoneydConfiguration
{

public:
    HoneydConfiguration();

    //XML Read Functions
    //calls main load functions
    bool LoadAllTemplates();

    // Returns the number of bits used in the mask when given in in_addr_t form
    static int GetMaskBits(in_addr_t mask);

    //Outputs the profile in a string formatted for direct insertion to a honeyd configuration file.
    std::string ProfileToString(profile* p);

    //Outputs the profile in a string formatted for direct insertion to a honeyd configuration file.
    // This function differs from ProfileToString in that it omits values incompatible with the loopback interface
    std::string DoppProfileToString(profile* p);

    //Saves the current configuration information to XML files
    bool SaveAllTemplates();
    //Writes the current configuration to honeyd configs
    bool WriteHoneydConfiguration(std::string path);

    //Setter for the directory to read from and write to
    void SetHomePath(std::string homePath);
    //Getter for the directory to read from and write to
    std::string GetHomePath();

    //Adds a port with the specified configuration into the port table
    //	portNum: Must be a valid port number (1-65535)
    //	isTCP: if true the port uses TCP, if false it uses UDP
    //	behavior: how this port treats incoming connections
    //	scriptName: this parameter is only used if behavior == SCRIPT, in which case it designates
    //		the key of the script it can lookup and execute for incoming connections on the port
    //	Note(s): If CleanPorts is called before using this port in a profile, it will be deleted
    //			If using a script it must exist in the script table before calling this function
    //Returns: the port name if successful and an empty string if unsuccessful
    std::string AddPort(uint16_t portNum, portProtocol isTCP, portBehavior behavior, std::string scriptName = "");

    // Some high level node creation methods

    // Add a node with static IP and static MAC
    bool AddNewNode(std::string profileName, std::string ipAddress, std::string macAddress, std::string interface, std::string subnet);
    bool AddNewNodes(std::string profileName, std::string ipAddress,std::string interface, std::string subnet, int numberOfNodes);


	std::vector<std::string> GetProfileChildren(std::string parent);

	std::vector<std::string> GetProfileNames();
	Nova::profile * GetProfile(std::string profileName);
	Nova::port * GetPort(std::string portName);

	std::vector<std::string> GetNodeNames();
	std::vector<std::string> GetSubnetNames();

	// Returns list of all available scripts
	std::vector<std::string> GetScriptNames();

	bool IsMACUsed(std::string mac);
	bool IsIPUsed(std::string ip);
	bool IsProfileUsed(std::string profileName);

	// Regenerates the MAC addresses for nodes of this profile
	void GenerateMACAddresses(std::string profileName);
	std::string GenerateUniqueMACAddress(std::string vendor);

	//Inserts the profile into the honeyd configuration
	//	profile: pointer to the profile you wish to add
	//	Returns (true) if the profile could be created, (false) if it cannot.
	bool AddProfile(profile * profile);

	//Updates the profile with any modified information
	//	Note: to modify inheritance use InheritProfile, just changing the parentProfile value and calling
	//		this function may leave a copy of the profile as a child of the old parent next load
	bool UpdateProfile(std::string profileName)
	{
		return UpdateProfileTree(profileName, ALL);
	}

    bool RenameProfile(std::string oldName, std::string newName);

    //Makes the profile named child inherit the profile named parent
    // child: the name of the child profile
    // parent: the name of the parent profile
    // Returns: (true) if successful, (false) if either name could not be found
    bool InheritProfile(std::string child, std::string parent);

    //Iterates over the profiles, recreating the entire property tree structure
    void UpdateAllProfiles();

	//Removes a profile and all associated nodes from the Honeyd configuration
	//	profileName: name of the profile you wish to delete
	// 	Returns: (true) if successful and (false) if the profile could not be found
	bool DeleteProfile(std::string profileName)
    {
    	return DeleteProfile(profileName, true);
    }

    //Deletes a single node, called from deleteNodes();
    bool DeleteNode(std::string nodeName);
    Node * GetNode(std::string nodeName);

    std::string GetNodeSubnet(std::string nodeName);
    bool EnableNode(std::string nodeName);
    bool DisableNode(std::string nodeName);
    void DisableProfileNodes(std::string profileName);

    //Checks for ports that aren't used and removes them from the table if so
    void CleanPorts();

    // This is only for the Javascript UI, avoid use here
	inline std::vector<port> GetPorts(std::string profile) {
		std::vector<port> ret;
		port p;

		for (uint i = 0; i < m_profiles[profile].ports.size(); i++)
		{
			p = m_ports[m_profiles[profile].ports.at(i).first];
			p.isInherited = m_profiles[profile].ports.at(i).second;
			ret.push_back(p);
		}

		return ret;
	}


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
    bool LoadScriptsTemplate();
    //load all ports
    bool LoadPortsTemplate();
    //load all profiles
    bool LoadProfilesTemplate();
    //load current honeyd configuration group
    bool LoadNodesTemplate();

    //set profile configurations
    bool LoadProfileSettings(boost::property_tree::ptree *ptr, profile *p);
    //add ports or subsystems
    bool LoadProfileServices(boost::property_tree::ptree *ptr, profile *p);
    //recursive descent down profile tree
    bool LoadProfileChildren(std::string parent);

    //Load stored subnets in ptr
    bool LoadSubnets(boost::property_tree::ptree *ptr);
    //Load stored honeyd nodes ptr
    bool LoadNodes(boost::property_tree::ptree *ptr);

    bool DeleteProfile(std::string profileName, bool originalCall);

	//Recreates the profile tree of ancestors, children or both
    //	Note: This needs to be called after making changes to a profile to update the hierarchy
    //	Returns (true) if successful and (false) if no profile with name 'profileName' exists
    bool UpdateProfileTree(std::string profileName, recursiveDirection direction);

    //Creates a ptree for a profile from scratch using the values found in the table
    //	name: the name of the profile you wish to create a new tree for
    //	Note: this only creates a leaf-node profile tree, after this call it will have no children.
    //		to put the children back into the tree and place the this new tree into the parent's hierarchy
    //		you must first call UpdateProfileTree(name, ALL);
    //	Returns (true) if successful and (false) if no profile with name 'profileName' exists
    bool CreateProfileTree(std::string profileName);

    std::string FindSubnet(in_addr_t ip);
};

}

#endif
