#ifndef TEMPLATE_REGISTRY_HPP
#define TEMPLATE_REGISTRY_HPP

#include "EntityTemplate.hpp"
#include "Position.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace tutorial
{
	class Entity;

	class TemplateRegistry
	{
	public:
		// Get singleton instance
		static TemplateRegistry& Instance();

		// Load templates from a JSON file
		void LoadFromFile(const std::string& filepath);

		// Load all templates from a directory (legacy format: multiple
		// templates per file)
		void LoadFromDirectory(const std::string& directory);

		// Load simplified templates from directory (one JSON per
		// template) type: "item" or "unit" - determines which parser to
		// use
		void LoadSimplifiedDirectory(const std::string& directory,
		                             const std::string& type);

		// Get a template by ID (returns nullptr if not found)
		const EntityTemplate* Get(const std::string& id) const;

		// Check if a template exists
		bool Has(const std::string& id) const;

		// Create an entity from a template
		std::unique_ptr<Entity> Create(const std::string& id,
		                               pos_t pos) const;

		// Clear all templates (useful for testing/reloading)
		void Clear();

		// Get all template IDs (useful for debugging)
		std::vector<std::string> GetAllIds() const;

	private:
		TemplateRegistry() =
		    default; // Private constructor for singleton
		~TemplateRegistry() = default;

		// Prevent copying
		TemplateRegistry(const TemplateRegistry&) = delete;
		TemplateRegistry& operator=(const TemplateRegistry&) = delete;

		std::unordered_map<std::string, EntityTemplate> templates_;
	};
} // namespace tutorial

#endif // TEMPLATE_REGISTRY_HPP
