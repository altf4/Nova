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
	INHERITED
	, NOT_INHERITED
	, NO_SUCH_KEY
};

enum portBehavior
{
	BLOCK = 0
	, RESET
	, OPEN
	, SCRIPT
	, TARPIT_OPEN
	, TARPIT_SCRIPT
};
enum recursiveDirection
{
	ALL = 0
	, UP
	, DOWN
};

enum portProtocol : bool
{
	UDP = 0
	, TCP
};

class HoneydConfiguration
{

public:

    //***************** REFACTORED FROM HERE *****************//

	//Basic constructor for the Honeyd Configuration object
	// Initializes the MAC vendor database and hash tables
	// *Note: To populate the object from the file system you must call LoadAllTemplates();
    HoneydConfiguration();

    //Attempts to populate the HoneydConfiguration object with the xml templates.
    // The configuration is saved and loaded relative to the homepath specificed by the Nova Configuration
    // Returns true if successful, false if loading failed.
    bool LoadAllTemplates();

    //This function takes the current values in the HoneydConfiguration and Config objects
    // 		and translates them into an xml format for persistent storage that can be
    // 		loaded at a later time by any HoneydConfiguration object
    // Returns true if successful and false if the save fails
    bool SaveAllTemplates();

    //Writes out the current HoneydConfiguration object to the Honeyd configuration file in the expected format
    // path: path in the file system to the desired HoneydConfiguration file
    // Returns true if successful and false if not
    bool WriteHoneydConfiguration(std::string path = "");

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

    //This function inserts a pre-created port into the HoneydConfiguration object
    //	pr: Port object you wish to add into the table
    //	Returns: the port name if successful and an empty string if unsuccessful
    std::string AddPort(Port pr);

    //This function creates a new Honeyd node based on the parameters given
    //	profileName: name of the existing NodeProfile the node should use
    //	ipAddress: string form of the IP address or the string "DHCP" if it should acquire an address using DHCP
    //	macAddress: string form of a MAC address or the string "RANDOM" if one should be generated each time Honeyd is run
    //	interface: the name of the physical or virtual interface the Honeyd node should be deployed on.
    //	subnet: the name of the subnet object the node is associated with for organizational reasons.
    //	Returns true if successful and false if not
    bool AddNewNode(std::string profileName, std::string ipAddress, std::string macAddress, std::string interface, std::string subnet);

    //This function adds a new node to the configuration based on the existing node.
    // Note* this function does not perform robust validation and is used primarily by the NodeManager,
    //	avoid using this otherwise
    bool AddPreGeneratedNode(Node &node);

    //This function allows us to add many nodes of the same type easily
    // *Note this function is very limited, for configuring large numbers of nodes you should use the NodeManager
    bool AddNewNodes(std::string profileName, std::string ipAddress, std::string interface, std::string subnet, int numberOfNodes);

    //This allows easy access to all children profiles of the parent
    // Returns a vector of strings containing the names of the children profile
	std::vector<std::string> GetProfileChildren(std::string parent);

	//This function allows easy access to all profiles
	// Returns a vector of strings containing the names of all profiles
	std::vector<std::string> GetProfileNames();

	//This function allows easy access to all nodes
	// Returns a vector of strings containing the names of all nodes
	std::vector<std::string> GetNodeNames();

	//This function allows easy access to all scripts
	// Returns a vector of strings containing the names of all scripts
	std::vector<std::string> GetScriptNames();

	//This function allows easy access to all generated profiles
	// Returns a vector of strings containing the names of all generated profiles
	std::vector<std::string> GetGeneratedProfileNames();

	//This function allows easy access to debug strings of all generated profiles
	// Returns a vector of strings containing debug outputs of all generated profiles
	std::vector<std::string> GeneratedProfilesStrings();

    //This function determines whether or not the given profile is empty
    // targetProfileKey: The name of the profile being inherited
    // Returns true, if valid parent and false if not
    // *Note: Used by auto configuration? may not be needed.
    bool CheckNotInheritingEmptyProfile(std::string parentName);

    //This function allows easy access to all auto-generated nodes.
    // Returns a vector of node names for each node on a generated profile.
	std::vector<std::string> GetGeneratedNodeNames();

	//This function allows access to NodeProfile objects by their name
	// profileName: the name or key of the NodeProfile
	// Returns a pointer to the NodeProfile object or NULL if the key doesn't
	NodeProfile *GetProfile(std::string profileName);

	//This function allows access to Port objects by their name
	// portName: the name or key of the Port
	// Returns a pointer to the Port object or NULL if the key doesn't exist
	Port GetPort(std::string portName);

	//This function allows the caller to find out if the given MAC string is taken by a node
	// mac: the string representation of the MAC address
	// Returns true if the MAC is in use and false if it is not.
	// *Note this function may have poor performance when there are a large number of nodes
	bool IsMACUsed(std::string mac);

	//This function allows the caller to find out if the given IP string is taken by a node
	// ip: the string representation of the IP address
	// Returns true if the IP is in use and false if it is not.
	// *Note this function may have poor performance when there are a large number of nodes
	bool IsIPUsed(std::string ip);

	//This function allows the caller to find out if the given profile is being used by a node
	// profileName: the name or key of the profile
	// Returns true if the profile is in use and false if it is not.
	// *Note this function may have poor performance when there are a large number of nodes
	// TODO - change this to check the m_nodeKeys vector in the NodeProfile objects to avoid table iteration
	bool IsProfileUsed(std::string profileName);

	//This function generates a MAC address that is currently not in use by any other node
	// vendor: string name of the MAC vendor from which to choose a MAC range from
	// Returns a string representation of MAC address or an empty string if the vendor is not valid
	std::string GenerateUniqueMACAddress(std::string vendor);

    //***************** TO HERE *****************//

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

    bool EnableNode(std::string nodeName);
    bool DisableNode(std::string nodeName);
    void DisableProfileNodes(std::string profileName);

    //Checks for ports that aren't used and removes them from the table if so
    void CleanPorts();

    // This is only for the Javascript UI, avoid use here
	inline std::vector<Port> GetPorts(std::string profile)
	{
		std::vector<Port> ret;
		Port p;

		if(!m_profiles.keyExists(profile))
		{
			return ret;
		}

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

    static std::string SanitizeProfileName(std::string pfilename);

    // When a profile's ethernet vendor is changed, we need to update the MAC addresses
    // for any node that uses that profile to reflect the change. This method will take
    // in a profile's name, find it in the hashmap and then look at all of the nodes that
    // it spawned and update the MAC address fields.
    //	  profileName: name of the profile whose nodes need updating.
    // Returns a bool indicating whether the update was successful or not.
    bool UpdateNodeMacs(std::string profileName);

    bool AddNewConfiguration(const std::string&, bool, const std::string&);

    bool SwitchToConfiguration(const std::string&);

    bool RemoveConfiguration(const std::string&);

    bool LoadConfigurations();

    std::vector<std::string> GetConfigurationsList();

	PortTable m_ports;
	ProfileTable m_profiles;
    NodeTable m_nodes;
    ScriptTable m_scripts;

    std::vector<std::string> m_configs;
    std::vector<std::string> m_groups;

    VendorMacDb m_macAddresses;

private:
    uint m_nodeProfileIndex;


    //Storing these trees allow for easy modification and writing of the XML files
    //Without having to reconstruct the tree from scratch.
    boost::property_tree::ptree m_groupTree;
    boost::property_tree::ptree m_portTree;
    boost::property_tree::ptree m_profileTree;
    boost::property_tree::ptree m_scriptTree;
    boost::property_tree::ptree m_nodesTree;
    boost::property_tree::ptree m_subnetTree;

    // ******************* Private Methods **************************
    //***************** REFACTORED FROM HERE *****************//


    //Loads Scripts from the xml template located relative to the currently set home path
    // Returns true if successful, false if not.
    bool LoadScriptsTemplate();

    //Loads Ports from the xml template located relative to the currently set home path
    // Returns true if successful, false if not.
    bool LoadPortsTemplate();

    //Loads NodeProfiles from the xml template located relative to the currently set home path
    // Returns true if successful, false if not.
    bool LoadProfilesTemplate();

    //Loads Nodes from the xml template located relative to the currently set home path
    // Returns true if successful, false if not.
    bool LoadNodesTemplate();

    //Iterates of the node table and populates the NodeProfiles with accessor keys to the node objects that use them.
    // Returns true if successful and false if it is unable to assocate a profile with an exisiting node.
    bool LoadNodeKeys();

    //***************** TO HERE *****************//

    //set profile configurations
    bool LoadProfileSettings(boost::property_tree::ptree *ptr, NodeProfile *p);
    //add ports or subsystems
    bool LoadProfileServices(boost::property_tree::ptree *ptr, NodeProfile *p);
    //recursive descent down profile tree
    bool LoadProfileChildren(std::string parent);

    //Load stored honeyd nodes ptr
    bool LoadNodes(boost::property_tree::ptree *ptr);

    //Removes a profile and all associated nodes from the Honeyd configuration
    //	profileName: name of the profile you wish to delete
    //	originalCall: used internally to designate the recursion's base condition, can old be set with
    //		private access. Behavior is undefined if the first DeleteProfile call has originalCall == false
    // 	Returns: (true) if successful and (false) if the profile could not be found
    bool DeleteProfile(std::string profileName, bool originalCall);

    void GetProfilesToDelete(std::string profileName, std::vector<std::string> &profilesToDelete);

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


    //***************** REFACTORED BELOW  HERE *****************//

    //This internal function recurses upward to determine whether or not the given profile has a personality
    // check: Reference to the profile to check
    // Returns true if there is a personality defined, false if not
    // *Note: Used by auto configuration? shouldn't be needed.
    bool RecursiveCheckNotInheritingEmptyProfile(const NodeProfile& check);

};

}

#endif
