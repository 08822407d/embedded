#include "ScreenPage.hpp"


// void ScreenPage::init() {

// }

// void ScreenPage::dispose() {
	
// }

// void ScreenPage::enterPage() {

// }

// void ScreenPage::exitPage() {

// }


void ScreenPage::addChild(ScreenPage *child) {
	child->_parent = this;
	this->_children.push_back(child);
}

const std::vector<ScreenPage *>& ScreenPage::getChildren() const {
	return this->_children;
}

ScreenPage* ScreenPage::getParent() const {
	return this->_parent;
}