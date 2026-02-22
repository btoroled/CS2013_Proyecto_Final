#ifndef INC_1_CATALOG_H
#define INC_1_CATALOG_H

#include "../../include/core/Movie.h"
#include "../../include/text/TextUtils.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <random>

class Catalog {
private:
    static bool parse_csv_row(std::istream& in, std::vector<std::string>& row);

public:
    std::vector<Movie> base_datos;
    bool load(const std::string& file);
    void printValidation();
};


#endif