#include "Classyfier.h"

#include <regex>

Classyfier::Classyfier(const std::vector<Symbol>& symbolRepo) 
    : symbols(symbolRepo) {}

Classyfier::Classyfier(const std::vector<Symbol>& symbolRepo, const std::vector<std::string>& blacklist) 
    : symbols(symbolRepo), blacklistClasses(blacklist) {}

void Classyfier::setSymbols(const std::vector<Symbol>& symbolRepo) {
    symbols = symbolRepo;
}

const std::vector<Symbol>& Classyfier::getSymbols() const {
    return symbols;
}

void Classyfier::setVTables(const std::vector<VTable>& vtableRepo) {
    vtables = vtableRepo;
}
const std::vector<VTable>& Classyfier::getVTables() const {
    return vtables;
}

void Classyfier::setBlacklistClasses(const std::vector<std::string>& blacklist) {
    blacklistClasses = blacklist;
}

const std::vector<std::string>& Classyfier::getBlacklist() const {
    return blacklistClasses;
}

const std::vector<Object*>& Classyfier::getObjects() const {
    return objects;
}

uint32 Classyfier::parse() {
	uint32 skipped = 0;
	for (const auto& symbol : symbols) {
		if (symbol) {
			if (!parse(symbol)) {
				++skipped;
			}
		}
	}

	return skipped;
}

bool Classyfier::parse(const Symbol& symbol) {
    const auto& values = Classyfier::extractValues(symbol.getSymbol());
	if (values.size() > 1) {
		if (!std::regex_match(values[0], std::regex("^[a-zA-Z_]{1}[a-zA-Z_0-9]+$"))) {
			return false;
		}

		for (const auto& it : blacklistClasses) {
			if (it == values[0]) {
				return true;
			}
		}

		std::string it;
		Object* parent = nullptr;
		for (std::size_t index = 0; index < values.size(); index++) {
			it = values[index];
			if (parent == nullptr) {
				for (const auto& object : objects) {
					if (object->getName() == it) {
						parent = object;
						break;
					}
				}

				if (parent == nullptr) {
					parent = new Object(Object::Type::NAMESPACE, it);
					objects.push_back(parent);
				}
			} else {
				Object* tmp = parent->getChild(it);
				if (tmp == nullptr) {
					if (index == values.size() - 1) {
						Object::Type type = it.find("(") != std::string::npos ? Object::Type::METHOD : Object::Type::FIELD;
						tmp = new Object(type, it);

						if (type == Object::Type::METHOD) {
							if ("~" + parent->getName() + "()" == it) {
								tmp->getSymbol().isDestructor = true;
							}

							if (std::regex_match(it, std::regex(parent->getName() + "\\(.+"))) {
								tmp->getSymbol().isConstructor = true;
							}
						}

						parent->setType(Object::Type::CLASS);
					} else {
						tmp = new Object(Object::Type::NAMESPACE, it);
					}

					parent->addChild(tmp);
				}

				parent = tmp;
			}
		}
	}

    return true;
}

std::vector<std::string> Classyfier::extractValues(const std::string& line) const {
	std::vector<std::string> values;
	std::string val;
	bool addMissingLine = false;
	for (const char& character : line) {
		if (character == ':' && !addMissingLine) {
			if (!val.empty()) {
				values.push_back(val);
				val.clear();
			}
		} else if (character == '<' && !addMissingLine) {
			addMissingLine = true;
			val += character;
		} else if (character == '(' && !addMissingLine) {
			addMissingLine = true;
			val += character;
		} else {
			val += character;
		}
	}

	if (!val.empty()) {
		values.push_back(val);
	}

	return std::move(values);
}