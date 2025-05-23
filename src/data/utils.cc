#include "data/utils.hh"
#include "osm/osmobjects.hh"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace osmobjects;

/// \namespace datautils
namespace datautils {

DataUtils::DataUtils(std::shared_ptr<Pq> db) {
    dbconn = db;
}

// Receives a dictionary of tags (key: value) and returns
// a JSONB string for doing an insert operation into the database.
std::string
DataUtils::buildTagsQuery(std::map<std::string, std::string> tags) const {
    if (tags.size() > 0) {
        std::string tagsStr = "jsonb_build_object(";
        int count = 0;
        for (auto it = std::begin(tags); it != std::end(tags); ++it) {
            ++count;
            // PostgreSQL has an argument LIMIT for functions (100 parameters max)
            // Because of this, when the count of key/value pairs reaches 50, 
            // a concatenation of multiple calls to the jsonb_build_object() function 
            // is needed.
            if (count == 50) {
                tagsStr.erase(tagsStr.size() - 1);
                tagsStr += ") || jsonb_build_object(";
                count = 0;
            }
            std::string tag_format = "'%s', '%s',";
            boost::format tag_fmt(tag_format);
            tag_fmt % dbconn->escapedString(dbconn->escapedJSON(it->first));
            tag_fmt % dbconn->escapedString(dbconn->escapedJSON(it->second));
            tagsStr += tag_fmt.str();
        }
        tagsStr.erase(tagsStr.size() - 1);
        return tagsStr + ")";
    } else {
        return "null";
    }
}

// Receives a list of Relation members and returns
// a JSONB string for doing an insert operation into the database.
std::string
DataUtils::buildMembersQuery(std::list<OsmRelationMember> members) const {
    if (members.size() > 0) {
        std::string membersStr = "'[";
        for (auto mit = std::begin(members); mit != std::end(members); ++mit) {
            membersStr += "{";
            std::string member_format = "\"%s\": \"%s\",";
            boost::format member_fmt(member_format);
            member_fmt % "role";
            member_fmt % mit->role;
            membersStr += member_fmt.str();
            member_fmt % "type";
            switch(mit->type) {
                case osmobjects::osmtype_t::way:
                    member_fmt % "way"; break;
                case osmobjects::osmtype_t::node:
                    member_fmt % "node"; break;
                case osmobjects::osmtype_t::relation:
                    member_fmt % "relation"; break;
                default:
                    member_fmt % "";
            }
            membersStr += member_fmt.str();
            membersStr += "\"ref\":";
            membersStr += std::to_string(mit->ref);
            membersStr += "},";
        }
        membersStr.erase(membersStr.size() - 1);

        return membersStr += "]'";
    } else {
        return "null";
    }
}

// Parses a JSON object from a string and return a map of key/value.
// This function is useful for parsing tags from a query result.
std::map<std::string, std::string>
DataUtils::parseJSONObjectStr(std::string input) const {
    std::map<std::string, std::string> obj;
    boost::property_tree::ptree pt;
    try {
        std::istringstream jsonStream(input);
        boost::property_tree::read_json(jsonStream, pt);
    } catch (const boost::property_tree::json_parser::json_parser_error& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return obj;
    }
    for (const auto& pair : pt) {
        obj[pair.first] = pair.second.get_value<std::string>();
    }
    return obj;
}

// Parses a JSON object from a string and return a vector of key/value maps
// This function is useful for parsing relation members from a query result.
std::vector<std::map<std::string, std::string>>
DataUtils::parseJSONArrayStr(std::string input) const {
    std::vector<std::map<std::string, std::string>> arr;
    boost::property_tree::ptree pt;
    try {
        std::istringstream jsonStream(input);
        boost::property_tree::read_json(jsonStream, pt);
    } catch (const boost::property_tree::json_parser::json_parser_error& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return arr;
    }

    for (const auto& item : pt) {
        std::map<std::string, std::string> obj;
        for (const auto& pair : item.second) {
            obj[pair.first] = pair.second.get_value<std::string>();
        }
        arr.push_back(obj);
    }

    return arr;
}

// Receives a string of comma separated values and
// returns a vector. This function is useful for
// getting a vector of references from a query result
std::vector<long>
DataUtils::arrayStrToVector(std::string refs_str) const {
    refs_str.erase(0, 1);
    refs_str.erase(refs_str.size() - 1);
    std::vector<long> refs;
    std::stringstream ss(refs_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        refs.push_back(std::stod(token));
    }
    return refs;
}

boost::posix_time::ptime
DataUtils::cleanTimeStr(std::string timestampStr) const {
    std::string cleanedTimeStr = timestampStr.substr(0, 19); // keep only "YYYY-MM-DD HH:MM:SS"
    std::replace(cleanedTimeStr.begin(), cleanedTimeStr.end(), ' ', 'T');
    return from_iso_extended_string(cleanedTimeStr);;
}

}