#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#include <gui/common/Enums.hpp>

Model::Model() : modelListener(0), elementID(0), previousScreen(Enums::Settings), hour(10), minute(20), minuteCookingTimer(0), secondCookingTimer(0)
{

}

void Model::tick()
{

}
