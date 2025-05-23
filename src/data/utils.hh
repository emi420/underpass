#include "data/pq.hh"
#include "osm/osmobjects.hh"

using namespace pq;
using namespace osmobjects;


/// \namespace datautils
namespace datautils {

class DataUtils {

    std::shared_ptr<Pq> dbconn;

    public:
        DataUtils();
        DataUtils(std::shared_ptr<Pq> dbconn);
        ~DataUtils(void){};

        // Receives a dictionary of tags (key: value) and returns
        // a JSONB string for doing an insert operation into the database.
        std::string
        buildTagsQuery(std::map<std::string, std::string> tags) const;

        // Receives a list of Relation members and returns
        // a JSONB string for doing an insert operation into the database.
        std::string
        buildMembersQuery(std::list<OsmRelationMember> members) const;

        // Parses a JSON object from a string and return a map of key/value.
        // This function is useful for parsing tags from a query result.
        std::map<std::string, std::string> parseJSONObjectStr(std::string input) const;

        // Parses a JSON object from a string and return a vector of key/value maps
        // This function is useful for parsing relation members from a query result.
        std::vector<std::map<std::string, std::string>> parseJSONArrayStr(std::string input) const;

        // Receives a string of comma separated values and
        // returns a vector. This function is useful for
        // getting a vector of references from a query result
        std::vector<long> arrayStrToVector(std::string refs_str) const;

        boost::posix_time::ptime cleanTimeStr(std::string timestampStr) const;

};

}