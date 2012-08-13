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

#include "../Defines.h"
#include "HoneydConfigTypes.h"
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

	//Basic constructor for the Honeyd Configuration object
	// Initializes the MAC vendor database and hash tables
	// *Note: To populate the object from the file system you must call LoadAllTemplates();
    HoneydConfiguration();

    //Attempts to populate the HoneydConfiguration object with the xml templates.
    // The configuration is saved and loaded relative to the homepath specificed by the Nova Configuration
    // Returns true if successful, false if loading failed.
    bool LoadAllTemplates();

    // This function takes in the raw byte form of a network mask and converts it to the number of bits
    // 	used when specifiying a subnet in the dots and slash notation. ie. 192.168.1.1/24
    // 	mask: The raw numerical form of the netmask ie. 255.255.255.0 -> 0xFFFFFF00
    // Returns an int equal to the number of bits that are 1 in the netmask, ie the example value for mask returns 24
    static int GetMaskBits(in_addr_t mask);

    //Outputs the NodeProfile in a string formate suitable for use in the Honeyd configuration file.
    // p: pointer to the profile you wish to create a Honeyd template for
    // Returns a string for direct inserting into a honeyd configuration file or an empty string if it fails.
    std::string ProfileToString(NodeProfile* p);

    //Outputs the NodeProfile in a string formate suitable for use in the Honeyd configuration file.
    // p: pointer to the profile you wish to create a Honeyd template for
    // Returns a string for direct inserting into a honeyd configuration file or an empty string if it fails.
    // *Note: This function differs from ProfileToString in that it omits values incompatible with the loopback
    //  interface and is used primarily for the Doppelganger node
    std::string DoppProfileToString(NodeProfile* p);

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
    std::string AddPort(uint16_t portNum, portProtocol isTCP, portBehavior behavior, std::string scriptName = "", std::string service = "");
    std::string AddPort(Port pr);

    // Some high level node creation methods

    // Add a node with static IP and static MAC
    bool AddNewNode(std::string profileName, std::string ipAddress, std::string macAddress, std::string interface, std::string subnet);
    bool AddNewNode(Node node);
    bool AddNewNodes(std::string profileName, std::string ipAddress,std::string interface, std::string subnet, int numberOfNodes);

    bool AddSubnet(const Subnet &add);

	std::vector<std::string> GetProfileChildren(std::string parent);

	std::vector<std::string> GetProfileNames();
	NodeProfile *GetProfile(std::string profileName);
	Port *GetPort(std::string portName);
	std::vector<std::string> GeneratedProfilesStrings();
	std::vector<std::string> GetGeneratedProfileNames();
	std::vector<std::string> GetGeneratedNodeNames();

	std::vector<std::string> GetNodeNames();
	std::vector<std::string> GetSubnetNames();

	// Returns list of all available scripts
	std::vector<std::string> GetScriptNames();

	bool IsMACUsed(std::string mac);
	bool IsIPUsed(std::string ip);
	bool IsProfileUsed(std::string profileName);

	// Regenerates the MAC addresses for nodes of this profile
	void UpdateMacAddressesOfProfileNodes(std::string profileName);
	std::string GenerateUniqueMACAddress(std::string vendor);

	//Inserts the profile into the honeyd configuration
	//	profile: pointer to the profile you wish to add
	//	Returns (true) if the profile could be created, (false) if it cannot.
	bool AddProfile(NodeProfile * profile);

	bool AddGroup(std::string groupName);

	std::vector<std::string> GetGroups();

	//Updates the profile with any modified information
	//	Note: to modify inheritance use InheritProfile, just changing the parentProfile value and calling
	//		this function may leave a copy of the profile as a child of the old parent next load
	bool UpdateProfile(std::string profileName)
	{
		CreateProfileTree(profileName);
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
    	return Nova::HoneydConfiguration::DeleteProfile(profileName, true);
    }

    //Deletes a single node, called from deleteNodes();
    bool DeleteNode(std::string nodeName);
    Node *GetNode(std::string nodeName);

    std::string GetNodeSubnet(std::string nodeName);
    bool EnableNode(std::string nodeName);
    bool DisableNode(std::string nodeName);
    void DisableProfileNodes(std::string profileName);

    //Checks for ports that aren't used and removes them from the table if so
    void CleanPorts();

    // This is only for the Javascript UI, avoid use here
	inline std::vector<Port> GetPorts(std::string profile) {
		std::vector<Port> ret;
		Port p;

		for (uint i = 0; i < m_profiles[profile].m_ports.size(); i++)
		{
			p = m_ports[m_profiles[profile].m_ports.at(i).first];
			p.m_isInherited = m_profiles[profile].m_ports.at(i).second.first;
			ret.push_back(p);
		}

		return ret;
	}

	ScriptTable &GetScriptTable();

    //Takes a ptree and loads and sub profiles (used in clone to extract children)
    bool LoadProfilesFromTree(std::string parent);

    bool CheckNotInheritingEmptyProfile(std::string parentName);

// TODO: this should be private eventually
public:
	SubnetTable m_subnets;
	PortTable m_ports;
	ProfileTable m_profiles;
    NodeTable m_nodes;

    std::vector<std::string> m_groups;

    VendorMacDb m_macAddresses;

private:
    std::string m_homePath;

    uint m_nodeProfileIndex;


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

    //Iterates of the node table and populates the NodeProfiles with accessor keys to the node objects that use them.
    // Returns true if successful and false if it is unable to assocate a profile with an exisiting node.
    bool LoadNodeKeys();

    //set profile configurations
    bool LoadProfileSettings(boost::property_tree::ptree *ptr, NodeProfile *p);
    //add ports or subsystems
    bool LoadProfileServices(boost::property_tree::ptree *ptr, NodeProfile *p);
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

    bool RecursiveCheckNotInheritingEmptyProfile(const NodeProfile& check);

    std::vector<Subnet> FindPhysicalInterfaces();
};

}

#endif
