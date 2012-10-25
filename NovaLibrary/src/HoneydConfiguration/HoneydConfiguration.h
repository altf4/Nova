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
#include "PortSet.h"
#include "PersonalityTree.h"

namespace Nova
{

class HoneydConfiguration
{

public:

	// This is a singleton class, use this to access it
	static HoneydConfiguration *Inst();

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

    //This function creates a new Honeyd node based on the parameters given
    //	profileName: name of the existing NodeProfile the node should use
    //	ipAddress: string form of the IP address or the string "DHCP" if it should acquire an address using DHCP
    //	macAddress: string form of a MAC address or the string "RANDOM" if one should be generated each time Honeyd is run
    //	interface: the name of the physical or virtual interface the Honeyd node should be deployed on.
    //	portSet: The PortSet to be used for the created node
    //	Returns true if successful and false if not
    bool AddNewNode(std::string profileName, std::string ipAddress, std::string macAddress,
    		std::string interface, PortSet *portSet);

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

	//Outputs the NodeProfile in a string format suitable for use in the Honeyd configuration file.
	// p: pointer to the profile you wish to create a Honeyd template for
	// Returns a string for direct inserting into a honeyd configuration file or an empty string if it fails.
	std::string ProfileToString(NodeProfile *p);

	//Outputs the NodeProfile in a string format suitable for use in the Honeyd configuration file.
	// p: pointer to the profile you wish to create a Honeyd template for
	// Returns a string for direct inserting into a honeyd configuration file or an empty string if it fails.
	// *Note: This function differs from ProfileToString in that it omits values incompatible with the loopback
	//  interface and is used strictly for the Doppelganger node
	std::string DoppProfileToString(NodeProfile *p);

	//Makes the profile named child inherit the profile named parent
	// child: the name of the child profile
	// parent: the name of the parent profile
	// Returns: (true) if successful, (false) if either name could not be found
	bool InheritProfile(std::string child, std::string parent);

	//This function allows the caller to find out if the given MAC string is taken by a node
	// mac: the string representation of the MAC address
	// Returns true if the MAC is in use and false if it is not.
	// *Note this function may have poor performance when there are a large number of nodes
	bool IsMACUsed(std::string mac);

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

    //Iterates over the profiles, recreating the entire property tree structure
    void UpdateAllProfiles();

	//Removes a profile and all associated nodes from the Honeyd configuration
	//	profileName: name of the profile you wish to delete
	// 	Returns: (true) if successful and (false) if the profile could not be found
	bool DeleteProfile(std::string profileName);

    //Deletes a single node, called from deleteNodes();
    bool DeleteNode(std::string nodeName);

    //TODO: Unsafe pointer access into table
    Node *GetNode(std::string nodeName);

    //Get a vector of PortSets associated with a particular profile
	std::vector<PortSet*> GetPortSets(std::string profileName);

	ScriptTable &GetScriptTable();

    static std::string SanitizeProfileName(std::string pfilename);

    // When a profile's ethernet vendor is changed, we need to update the MAC addresses
    // for any node that uses that profile to reflect the change. This method will take
    // in a profile's name, find it in the hashmap and then look at all of the nodes that
    // it spawned and update the MAC address fields.
    //	  profileName: name of the profile whose nodes need updating.
    // Returns a bool indicating whether the update was successful or not.
    bool UpdateNodeMacs(std::string profileName);

	PersonalityTree m_profiles;
    NodeTable m_nodes;

    std::vector<std::string> m_groups;

    VendorMacDb m_macAddresses;

private:

    //Basic constructor for the Honeyd Configuration object
	// Initializes the MAC vendor database and hash tables
	// *Note: To populate the object from the file system you must call LoadAllTemplates();
	HoneydConfiguration();

	//Write the current list of Port Sets out to ports.xml
	//	root - The root node of the (sub)tree that you wish to write for
	//	returns - True on success, false on error
	bool WritePortsToXML(PersonalityTreeItem *root);

	static HoneydConfiguration *m_instance;

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

    //set profile configurations
    bool LoadProfileSettings(boost::property_tree::ptree *ptr, NodeProfile *p);
    //add ports or subsystems
    bool LoadProfileServices(boost::property_tree::ptree *ptr, NodeProfile *p);
    //recursive descent down profile tree
    bool LoadProfileChildren(std::string parent);

    //Load stored honeyd nodes ptr
    bool LoadNodes(boost::property_tree::ptree *ptr);

    void GetProfilesToDelete(std::string profileName, std::vector<std::string> &profilesToDelete);

	//Recreates the profile tree of ancestors, children or both
    //	Note: This needs to be called after making changes to a profile to update the hierarchy
    //	Returns (true) if successful and (false) if no profile with name 'profileName' exists
    bool UpdateProfileTree(std::string profileName, RecursiveDirection direction);

    //Creates a ptree for a profile from scratch using the values found in the table
    //	name: the name of the profile you wish to create a new tree for
    //	Note: this only creates a leaf-node profile tree, after this call it will have no children.
    //		to put the children back into the tree and place the this new tree into the parent's hierarchy
    //		you must first call UpdateProfileTree(name, ALL);
    //	Returns (true) if successful and (false) if no profile with name 'profileName' exists
    bool CreateProfileTree(std::string profileName);

};

}

#endif
