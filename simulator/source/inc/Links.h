#pragma once
#include <vector>
#include <map>
#include <unordered_map>
typedef unsigned int uint;

template<class T, class U>
std::vector<U> mltimap2vector(std::multimap<T, U>& map, T key)
{
    auto i = map.equal_range(key);
    std::vector<U> intlist;

    for (auto it2 = i.first; it2 != i.second; ++it2)
    {
        intlist.push_back(it2->second);
    }
    return intlist;
}


struct MedmiumLoad
{
    uint sta;
    double load;
    MedmiumLoad(uint s, double d) : sta(s), load(d) {}
};

class Link
{
private:
    std::multimap<uint, uint> hashlink;
    std::unordered_map<uint, std::vector<MedmiumLoad>> loaddata;
public:
    inline void create(uint source, uint destin)
    {
        hashlink.emplace(source, destin);
        hashlink.emplace(destin, source);
    }
    inline std::vector<uint> dest(uint source)
    {
        return mltimap2vector(hashlink, source);
    }
    inline void setload(uint source, uint dest, double load)
    {
        loaddata[source].emplace_back(dest, load);
    }
    inline std::vector<double> getload(uint sta)
    {
        std::vector<double> output;
        for (auto& dest : loaddata[sta])
        {
            output.emplace_back(dest.load);
        }
        return output;
    }
    std::string to_string()
    {
        std::string out = "";
        for (auto& link : hashlink)
        {
            out += link.first + ":" + link.second + ',';
        }
        out.pop_back();
        return out;
    }

};
