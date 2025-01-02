#include "ScreenPage.hpp"



void ScreenPage::init() {

}

void ScreenPage::dispose() {
	
}

void ScreenPage::enterPage() {

}

void ScreenPage::exitPage() {

}


void ScreenPage::addChild(std::shared_ptr<ScreenPage> child) {
	child->_parent = this;
	this->_children.push_back(std::move(child));
}

const std::vector<std::shared_ptr<ScreenPage>>& ScreenPage::getChildren() const {
	return this->_children;
}

ScreenPage* ScreenPage::getParent() const {
	return this->_parent;
}