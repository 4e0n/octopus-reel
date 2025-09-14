
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>

static inline void replace_sep(std::string& s) {
    size_t pos = 0;
    while ((pos = s.find(" - ", pos)) != std::string::npos) {
        s.replace(pos, 3, " ");
    }
}

int main() {
    std::ifstream in("heeg.txt");
    std::ofstream out("heeg.raw", std::ios::binary);

    const unsigned numAmps = 2;
    const unsigned chansPerAmp = 66;

    // write header
    out.write((char*)&numAmps, 4);
    out.write((char*)&chansPerAmp, 4);

    std::string line;
    unsigned lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty()) continue;

        replace_sep(line);

        std::istringstream iss(line);

        unsigned long long offset = 0;
        iss >> offset;

        std::vector<float> vals;
        vals.reserve(numAmps * chansPerAmp);
        float v;
        while (iss >> v) {
            vals.push_back(v / 1e6f);   // µV → V
        }

        if (vals.size() != numAmps * chansPerAmp) {
            std::cerr << "Bad line " << lineNo
                      << ", got " << vals.size()
                      << " floats, expected "
                      << numAmps * chansPerAmp << "\n";
            continue;
        }

        float trigger = 0.0f;

        out.write((char*)&offset, sizeof(offset));
        out.write((char*)vals.data(), vals.size() * sizeof(float));
        out.write((char*)&trigger, sizeof(trigger));
    }
}
