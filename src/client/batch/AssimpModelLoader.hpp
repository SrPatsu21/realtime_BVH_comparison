#pragma once

#include <string>

#include "mesh/MeshImportData.hpp"

class AssimpModelLoader
{
public:

    static MeshImportData load(
        const std::string& path
    );
};