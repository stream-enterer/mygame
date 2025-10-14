#include "TemplateRegistry.hpp"

#include "Entity.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace tutorial
{
    TemplateRegistry& TemplateRegistry::Instance()
    {
        static TemplateRegistry instance;
        return instance;
    }

    void TemplateRegistry::LoadFromFile(const std::string& filepath)
    {
        // Open file
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open template file: "
                                     + filepath);
        }

        // Parse JSON
        json j;
        try
        {
            file >> j;
        }
        catch (const json::parse_error& e)
        {
            throw std::runtime_error("JSON parse error in " + filepath + ": "
                                     + e.what());
        }

        // Each top-level key is a template ID
        for (auto& [id, templateJson] : j.items())
        {
            try
            {
                EntityTemplate tpl = EntityTemplate::FromJson(id, templateJson);
                templates_[id] =
                    tpl; // Last-wins: overwrites if ID already exists
                std::cout << "[TemplateRegistry] Loaded template: " << id
                          << std::endl;
            }
            catch (const std::exception& e)
            {
                // Continue loading other templates even if one fails
                std::cerr << "[TemplateRegistry] Error loading template '" << id
                          << "' from " << filepath << ": " << e.what()
                          << std::endl;
            }
        }
    }

    void TemplateRegistry::LoadFromDirectory(const std::string& directory)
    {
        if (!fs::exists(directory))
        {
            throw std::runtime_error("Directory does not exist: " + directory);
        }

        if (!fs::is_directory(directory))
        {
            throw std::runtime_error("Path is not a directory: " + directory);
        }

        // Iterate through all .json files in directory
        for (const auto& entry : fs::directory_iterator(directory))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                std::string filepath = entry.path().string();
                std::cout << "[TemplateRegistry] Loading file: " << filepath
                          << std::endl;

                try
                {
                    LoadFromFile(filepath);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "[TemplateRegistry] Failed to load "
                              << filepath << ": " << e.what() << std::endl;
                    // Continue loading other files
                }
            }
        }
    }

    const EntityTemplate* TemplateRegistry::Get(const std::string& id) const
    {
        auto it = templates_.find(id);
        if (it != templates_.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool TemplateRegistry::Has(const std::string& id) const
    {
        return templates_.find(id) != templates_.end();
    }

    std::unique_ptr<Entity> TemplateRegistry::Create(const std::string& id,
                                                     pos_t pos) const
    {
        const EntityTemplate* tpl = Get(id);
        if (!tpl)
        {
            throw std::runtime_error("Template not found: " + id);
        }
        return tpl->CreateEntity(pos);
    }

    void TemplateRegistry::Clear()
    {
        templates_.clear();
        std::cout << "[TemplateRegistry] Cleared all templates" << std::endl;
    }

    std::vector<std::string> TemplateRegistry::GetAllIds() const
    {
        std::vector<std::string> ids;
        ids.reserve(templates_.size());
        for (const auto& [id, tpl] : templates_)
        {
            ids.push_back(id);
        }
        return ids;
    }
} // namespace tutorial
