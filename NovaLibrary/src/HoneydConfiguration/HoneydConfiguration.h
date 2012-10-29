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


	//**********************
	//* File IO Operations *
	//**********************

    //Writes out the current HoneydConfiguration object to the Honeyd configuration file in the expected format
    // path: path in the file system to the desired HoneydConfiguration file
    // Returns true if successful and false if not
    bool WriteHoneydConfiguration(std::string groupName, std::string path = "");

    //This function takes the current values in the HoneydConfiguration and Config objects
    // 		and translates them into an xml format for persistent storage that can be
    // 		loaded at a later time by any HoneydConfiguration object
    // Returns true if successful and false if the save fails
    bool WriteAllTemplatesToXML();

	//Write the current respective lists out to <template>.xml
    //	If you want to write all of them conventiently, run WriteAllTemplatesToXML()
	//	returns - True on success, false on error
	bool WriteScriptsToXML();
	bool WriteNodesToXML();
	bool WriteProfilesToXML();

    //Populates the HoneydConfiguration object with the xml templates.
    // The configuration is saved and loaded relative to the homepath specificed by the Nova Configuration
    // Returns true if successful, false if loading failed.
    bool ReadAllTemplatesXML();

    //Loads respective template from the xml template file located relative to the currently set home path
	// Returns true if successful, false on error
    bool ReadScriptsXML();
    bool ReadNodesXML();
    bool ReadProfilesXML();

	//*****************************
	//* Editing of Configurations *
	//*****************************

    //This function creates a new Honeyd node based on the parameters given
    //	profileName: name of the existing NodeProfile the node should use
    //	ipAddress: string form of the IP address or the string "DHCP" if it should acquire an address using DHCP
    //	macAddress: string form of a MAC address or the string "RANDOM" if one should be generated each time Honeyd is run
    //	interface: the name of the physical or virtual interface the Honeyd node should be deployed on.
    //	portSet: The PortSet to be used for the created node
    //	Returns true if successful and false if not
    bool AddNewNode(std::string profileName, std::string ipAddress, std::string macAddress,
    		std::string interface, PortSet *portSet);

	//This function allows access to NodeProfile objects by their name
	// profileName: the name or key of the NodeProfile
	// Returns a pointer to the NodeProfile object or NULL if the key doesn't
	NodeProfile *GetProfile(std::string profileName);

	//Inserts the profile into the honeyd configuration
	//	profile: pointer to the profile you wish to add
	//	Returns (true) if the profile could be created, (false) if it cannot.
	bool AddProfile(NodeProfile * profile);

	bool AddGroup(std::string groupName);

	std::vector<std::string> GetGroups();


	//Removes a profile and all associated nodes from the Honeyd configuration
	//	profileName: name of the profile you wish to delete
	// 	Returns: (true) if successful and (false) if the profile could not be found
	bool DeleteProfile(std::string profileName);

    //Deletes a single node, called from deleteNodes();
    bool DeleteNode(std::string nodeName);

    //TODO: Unsafe pointer access into table
    Node *GetNode(std::string nodeName);


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

    // This function takes in the raw byte form of a network mask and converts it to the number of bits
    // 	used when specifiying a subnet in the dots and slash notation. ie. 192.168.1.1/24
    // 	mask: The raw numerical form of the netmask ie. 255.255.255.0 -> 0xFFFFFF00
    // Returns an int equal to the number of bits that are 1 in the netmask, ie the example value for mask returns 24
    static int GetMaskBits(in_addr_t mask);

	//This function allows the caller to find out if the given MAC string is taken by a node
	// mac: the string representation of the MAC address
	// Returns true if the MAC is in use and false if it is not.
	// *Note this function may have poor performance when there are a large number of nodes
	bool IsMACUsed(std::string mac);

    bool RenameProfile(std::string oldName, std::string newName);

    //Get a vector of PortSets associated with a particular profile
	std::vector<PortSet*> GetPortSets(std::string profileName);

	ScriptTable &GetScriptTable();

    static std::string SanitizeProfileName(std::string pfilename);

	PersonalityTree m_profiles;

	//A vector of groups (the pair)
	// Each pair is a group, with the first string representing the group name, and the second Table being a table of nodes
	std::vector< std::pair<std::string, NodeTable> > m_nodes;

    VendorMacDb m_macAddresses;

private:

	static HoneydConfiguration *m_instance;

    //Basic constructor for the Honeyd Configuration object
	// Initializes the MAC vendor database and hash tables
	// *Note: To populate the object from the file system you must call ReadAllTemplates();
	HoneydConfiguration();

	//Helper function called by WriteProfilesToXML - Writes the profiles out to m_profileTree
	bool WriteProfilesToXML_helper(PersonalityTreeItem *root);

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

    //set profile configurations
    bool LoadProfileSettings(boost::property_tree::ptree *ptr, NodeProfile *p);
    //add ports or subsystems
    bool LoadProfileServices(boost::property_tree::ptree *ptr, NodeProfile *p);

    //Depth first traversal through ptree to read profiles
    PersonalityTreeItem *ReadProfilesXML_helper(boost::property_tree::ptree &ptree, PersonalityTreeItem *parent);

};

}

#endif
