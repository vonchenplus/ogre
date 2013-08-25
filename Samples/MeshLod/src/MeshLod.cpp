#include "MeshLod.h"

using namespace Ogre;
using namespace OgreBites;

#include "OgreLodConfigSerializer.h"
#include "OgrePixelCountLodStrategy.h"
#include "OgreQueuedProgressiveMeshGenerator.h"
#include "OgreProgressiveMeshGenerator.h"
#include "OgreMeshSerializer.h"

Sample_MeshLod::Sample_MeshLod() :
#if SHOW_MESH_HULL
	mHullEntity(0),
#endif
	mMeshEntity(0)
{
	mInfo["Title"] = "Mesh Lod";
	mInfo["Description"] = "Shows how to add Lod levels to a mesh using the ProgressiveMesh class.";
	mInfo["Thumbnail"] = "thumb_meshlod.png";
	mInfo["Category"] = "Unsorted";
}

void Sample_MeshLod::setupContent()
{
	mCameraMan->setStyle(CS_ORBIT);

	mSceneMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));  // set ambient light

	// make the scene's main light come from above
	Light* l = mSceneMgr->createLight();
	l->setType(Light::LT_DIRECTIONAL);
	l->setDirection(Vector3::NEGATIVE_UNIT_Y);

	// create a node for the model
	mMeshNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
#if SHOW_MESH_HULL
	mHullNode = mMeshNode->createChildSceneNode();
	mHullNode->scale(1.001,1.001,1.001);
#endif
	PMInjector::getSingleton().setInjectorListener(this);

	// setup gui
	setupControls();

	// load mesh
	changeSelectedMesh("sinbad.mesh");
}

void Sample_MeshLod::cleanupContent()
{
	PMInjector::getSingleton().removeInjectorListener();
	if(mMeshEntity){
		mSceneMgr->destroyEntity(mMeshEntity);
		mMeshEntity = 0;
		saveConfig();
	}
	cleanupControls();
}

void Sample_MeshLod::setupControls( int uimode /*= 0*/ )
{
	cleanupControls();

	SelectMenu* objectType = mTrayMgr->createLongSelectMenu(TL_TOPLEFT, "cmbModels", "Model:", 150, 8);
	objectType->addItem("sinbad.mesh");
	objectType->addItem("ogrehead.mesh");
	objectType->addItem("knot.mesh");
	objectType->addItem("fish.mesh");
	objectType->addItem("penguin.mesh");
	objectType->addItem("ninja.mesh");
	objectType->addItem("dragon.mesh");
	objectType->addItem("athene.mesh");
	objectType->addItem("sibenik.mesh");

	// Add all meshes from popular:
	StringVectorPtr meshes = ResourceGroupManager::getSingleton().findResourceNames("Popular", "*.mesh");
	StringVector::iterator it, itEnd;
	it = meshes->begin();
	itEnd = meshes->end();
	for(; it != itEnd; it++){
		objectType->addItem(*it);
	}

	// Basic options:
	mWireframe = mTrayMgr->createCheckBox(TL_TOPLEFT, "chkShowWireframe", "Show wireframe", 200);
	mUseVertexNormals = mTrayMgr->createCheckBox(TL_TOPLEFT, "chkUseVertexNormals", "Use vertex normals", 200);
	mOutsideWeightSlider = mTrayMgr->createThickSlider(TL_TOPLEFT, "sldOutsideWeight", "Weighten outside", 200, 50, 0, 100, 101);
	mOutsideWalkAngle = mTrayMgr->createThickSlider(TL_TOPLEFT, "sldOutsideWalkAngle", "Outside angle", 200, 50, -1, 1, 201);
	mReductionSlider = mTrayMgr->createThickSlider(TL_TOPLEFT, "sldReductionValue", "Reduced vertices", 200, 50, 0, 100, 101);
	mTrayMgr->createButton(TL_TOPLEFT, "btnReduceMore","Reduce More");
	mTrayMgr->createButton(TL_TOPLEFT, "btnReduceLess","Reduce Less");

	// Level options:
	mDistanceLabel = mTrayMgr->createLabel(TL_TOPRIGHT, "lblDistance", "Distance: ", 250);
	mLodLevelList = mTrayMgr->createLongSelectMenu(TL_TOPRIGHT, "cmbLodLevels", "Lod level:", 150, 4);
	mTrayMgr->createButton(TL_TOPRIGHT, "btnRemoveSelectedLodLevel","Remove level", 200);
	mTrayMgr->createButton(TL_TOPRIGHT, "btnAddLodLevel","Add level", 200);

	// Serializer options:
	mTrayMgr->createButton(TL_TOPRIGHT, "btnShowAll", "Show all levels", 200);
	mTrayMgr->createButton(TL_TOPRIGHT, "btnSaveMesh", "Save mesh", 200);
	mTrayMgr->createButton(TL_TOPRIGHT, "btnShowMesh", "Show Lod from mesh", 200);
	mTrayMgr->createButton(TL_TOPRIGHT, "btnAutoconfigure", "Show autoconfigured mesh", 200);

	// Profile options
	mProfileList = mTrayMgr->createLongSelectMenu(TL_TOPRIGHT, "cmbProfiledVertices", "Profile:", 180, 4);
	mTrayMgr->createButton(TL_TOPRIGHT, "btnRemoveFromProfile","Remove from profile", 200.0);
	mTrayMgr->createButton(TL_TOPRIGHT, "btnAddToProfile","Add to profile", 200.0);

	//mTrayMgr->createTextBox(TL_TOPRIGHT, "Help","Help", 200, 200)
	//	->setText("The last reduced vertex is the selected vertex. Use the slider to select the vertex, then decide to keep or remove it. You can export the Lod buffers into the .mesh file after configuration.");

	mTrayMgr->showCursor();
}

void Sample_MeshLod::cleanupControls()
{
	mTrayMgr->clearTray(TL_TOPLEFT);
	mTrayMgr->clearTray(TL_TOPRIGHT);
	mTrayMgr->clearTray(TL_TOP);
}

void Sample_MeshLod::changeSelectedMesh( const String& name )
{
	if(mMeshEntity){
		mSceneMgr->destroyEntity(mMeshEntity);
		mMeshEntity = 0;
		saveConfig();
	}
	mLodConfig.mesh = MeshManager::getSingleton().load(name, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	if(mLodConfig.mesh->getBounds().isNull() || mLodConfig.mesh->getBoundingSphereRadius() == 0.0){
		mTrayMgr->showOkDialog("Error", "Failed to load mesh!");
		return;
	}
	mMeshEntity = mSceneMgr->createEntity(name, mLodConfig.mesh);
	mMeshNode->attachObject(mMeshEntity);
	mCamera->setPosition(Vector3(0, 0, 0));
	mCamera->moveRelative(Vector3(0, 0, mLodConfig.mesh->getBoundingSphereRadius() * 2));
	mCamera->setNearClipDistance(mLodConfig.mesh->getBoundingSphereRadius() / 16);
	mCamera->setFarClipDistance(mLodConfig.mesh->getBoundingSphereRadius() * 256);

	size_t vertexCount = getUniqueVertexCount(mLodConfig.mesh);
	mReductionSlider->setRange(0,vertexCount,vertexCount+1,false);
	mOutsideWeightSlider->setValue(0, false);
	mOutsideWalkAngle->setValue(0, false);
	mLodLevelList->clearItems();
	mWorkLevel.distance = std::numeric_limits<Real>::max();
	mWorkLevel.reductionMethod = LodLevel::VRM_CONSTANT;
	mWorkLevel.reductionValue = 0.0;

	loadConfig();

	if(mLodLevelList->getNumItems() > 0){
		loadLodLevel(mLodLevelList->getSelectionIndex());
	} else {
		loadUserLod();
	}
}

bool Sample_MeshLod::loadConfig()
{
	mLodConfig.advanced = LodConfig::Advanced();
	mLodConfig.strategy = PixelCountLodStrategy::getSingletonPtr();
	mLodConfig.levels.clear();
	mLodConfig.advanced.profile.clear();

	// The mesh should already be set.
	assert(mLodConfig.mesh.get());

	std::string filename(mLodConfig.mesh->getName());
	filename += ".lodconfig";
	LodConfigSerializer lcs;
	lcs.importLodConfig(&mLodConfig, filename);

	mLodLevelList->clearItems();
	for(size_t i = 0; i < mLodConfig.levels.size(); i++){
		mLodLevelList->addItem(StringConverter::toString(mLodConfig.levels[i].distance) + "px");
	}

	mProfileList->clearItems();
	for(size_t i = 0; i < mLodConfig.advanced.profile.size(); i++){
		mProfileList->addItem(StringConverter::toString(mLodConfig.advanced.profile[i].src));
	}

	mUseVertexNormals->setChecked(mLodConfig.advanced.useVertexNormals, false);
	mOutsideWeightSlider->setValue(std::sqrt(mLodConfig.advanced.outsideWeight), false);
	mOutsideWalkAngle->setValue(mLodConfig.advanced.outsideWalkAngle, false);
	return true;
}

void Sample_MeshLod::saveConfig()
{
	std::string filename(mLodConfig.mesh->getName());
	filename += ".lodconfig";
	LodConfigSerializer lcs;
	lcs.exportLodConfig(mLodConfig, filename);
}

void Sample_MeshLod::loadAutomaticLod()
{
	// Remove outdated Lod requests to reduce delay.
	PMWorker::getSingleton().clearPendingLodRequests();
	QueuedProgressiveMeshGenerator pm;
	//pm.generateAutoconfiguredLodLevels(mLodConfig.mesh);
	LodConfig lodConfig;
	pm.getAutoconfig(mLodConfig.mesh, lodConfig);
	lodConfig.advanced.profile = mLodConfig.advanced.profile;
	lodConfig.advanced.useVertexNormals = mLodConfig.advanced.useVertexNormals;
	pm.generateLodLevels(lodConfig);
}

void Sample_MeshLod::loadUserLod( bool useWorkLod )
{
	if(mLodConfig.mesh->getBoundingSphereRadius() == 0){
		// failed to load mesh
		return;
	}
	if(mLodConfig.levels.empty()){
		useWorkLod = true;
	}
	mTrayMgr->destroyAllWidgetsInTray(TL_TOP);
	// Remove outdated Lod requests to reduce delay.
	PMWorker::getSingleton().clearPendingLodRequests();
	//QueuedProgressiveMeshGenerator pm; // Threaded
	ProgressiveMeshGenerator pm; // Non-threaded
	if(!useWorkLod){
		pm.generateLodLevels(mLodConfig);
		forceLodLevel(-1);
	} else {
		LodConfig config(mLodConfig);
		config.levels.clear();
		config.levels.push_back(mWorkLevel);
		pm.generateLodLevels(config);
		forceLodLevel(1);
	}
	getUniqueVertexCount(mLodConfig.mesh);
}
void Sample_MeshLod::forceLodLevel(int lodLevelID, bool forceDelayed)
{
	mForcedLodLevel = lodLevelID;
	if(!forceDelayed){
		if(lodLevelID == -1 || mLodConfig.mesh->getNumLodLevels() <= 1) {
			// Clear forced Lod level
			mMeshEntity->setMeshLodBias(1.0, 0, std::numeric_limits<unsigned short>::max());
		} else {
			mMeshEntity->setMeshLodBias(1.0, lodLevelID, lodLevelID);
		}
	}
}
size_t Sample_MeshLod::getUniqueVertexCount( MeshPtr mesh )
{
#if SHOW_MESH_HULL
	const String meshName("ConvexHull.mesh");
	if(mHullEntity){
		mHullNode->detachObject(mHullEntity);
		mSceneMgr->destroyEntity(mHullEntity);
		// Removes from the resources list.
		MeshManager::getSingleton().remove(meshName);
		mHullEntity = NULL;
	}
#endif
	// The vertex buffer contains the same vertex position multiple times.
	// To get the count of the vertices, which has unique positions, we can use progressive mesh.
	// It is constructing a mesh grid at the beginning, so if we reduce 0%, we will get the unique vertex count.
	LodConfig lodConfig;
	lodConfig.mesh = mesh;
	lodConfig.strategy = PixelCountLodStrategy::getSingletonPtr();
	LodLevel lodLevel;
	lodLevel.distance = 0;
	lodLevel.reductionMethod = LodLevel::VRM_PROPORTIONAL;
	lodLevel.reductionValue = 0.0;
	lodConfig.levels.push_back(lodLevel);
	ProgressiveMeshGenerator pm;
	pm.generateLodLevels(lodConfig);
#if SHOW_MESH_HULL
	mHullEntity = mSceneMgr->createEntity(pm._generateConvexHull(meshName, (int)mWorkLevel.reductionValue - 1));
	mHullNode->attachObject(mHullEntity);
#endif
	return lodConfig.levels[0].outUniqueVertexCount;
}

void Sample_MeshLod::addLodLevel()
{
	LodLevel lvl;
	lvl.distance = getCameraDistance();
	lvl.reductionMethod = LodLevel::VRM_CONSTANT;
	lvl.reductionValue = mReductionSlider->getValue();
	Real distepsilon = lvl.distance + lvl.distance * 0.001;
	size_t i = 0;
	bool addLevel = true;
	for(; i < mLodConfig.levels.size(); i++){
		if(mLodConfig.levels[i].distance < distepsilon){
			addLevel = false;
			break;
		}
	}
	if(/*mLodConfig.levels.empty() || */addLevel){
		mLodConfig.levels.push_back(lvl);
		mLodLevelList->addItem(StringConverter::toString(lvl.distance) + "px");
		mLodLevelList->selectItem(mLodLevelList->getNumItems() - 1, false);
	} else {
		mLodConfig.levels.insert(mLodConfig.levels.begin() + i, lvl);
		mLodLevelList->insertItem(i, StringConverter::toString(lvl.distance) + "px");
		mLodLevelList->selectItem(i, false);
	}
}

void Sample_MeshLod::loadLodLevel( int id )
{
	assert(mLodConfig.levels[id].reductionMethod == LodLevel::VRM_CONSTANT);
	mWorkLevel = mLodConfig.levels[id];
	mReductionSlider->setValue(mWorkLevel.reductionValue, false);
	mLodLevelList->selectItem(id, false);
	moveCameraToPixelDistance(mWorkLevel.distance);
	loadUserLod();
}

void Sample_MeshLod::removeLodLevel()
{
	if(mLodConfig.levels.empty()){
		return;
	}
	int selectedLevel = mLodLevelList->getSelectionIndex();
	mLodConfig.levels.erase(mLodConfig.levels.begin() + selectedLevel);
	mLodLevelList->removeItem(selectedLevel);
}

Real Sample_MeshLod::getCameraDistance()
{
	if(mLodConfig.mesh->getBoundingSphereRadius() != 0.0){
		return PixelCountLodStrategy::getSingleton().getValue(mMeshEntity, mCameraMan->getCamera());
	} else {
		return 0.0;
	}
}

void Sample_MeshLod::moveCameraToPixelDistance( Real pixels )
{
	PixelCountLodStrategy& strategy = PixelCountLodStrategy::getSingleton();
	Real distance = mLodConfig.mesh->getBoundingSphereRadius() * 4;
	const Real epsilon = pixels * 0.000001;
	const int iterations = 64;
	mCamera->setPosition(Vector3(0, 0, 0));
	mCamera->moveRelative(Vector3(0, 0, distance));
	// We need to find a distance, which is bigger then requested
	for(int i=0;i<iterations;i++){
		Real curPixels = strategy.getValue(mMeshEntity, mCameraMan->getCamera());
		if (curPixels > pixels) {
			distance *= 2.0;
			mCamera->moveRelative(Vector3(0, 0, distance));
		} else {
			break;
		}
	}
	// Binary search for distance
	for(int i=0;i<iterations;i++){
		Real curPixels = strategy.getValue(mMeshEntity, mCameraMan->getCamera());
		if(std::abs(curPixels - pixels) < epsilon){
			break;
		}
		distance /= 2;
		if (curPixels > pixels) {
			// move camera further
			mCamera->moveRelative(Vector3(0, 0, distance));
		} else {
			// move camera nearer
			mCamera->moveRelative(Vector3(0, 0, -distance));
		}
	}
}

bool Sample_MeshLod::getResourceFullPath(MeshPtr& mesh, String& outPath)
{
	ResourceGroupManager& resourceGroupMgr = ResourceGroupManager::getSingleton();
	String group = mesh->getGroup();
	// If we don't add * to the name, the pattern matcher will not find it.
	String name = "*" + mLodConfig.mesh->getName();
	FileInfoListPtr locPtr = resourceGroupMgr.findResourceFileInfo(group, name);
	assert(locPtr->size() == 1);

	FileInfo& info = locPtr->at(0);

	outPath = info.archive->getName();
	if(outPath.back() != '/' && outPath.back() != '\\')
		outPath += '/';
	outPath += info.path;
	if(outPath.back() != '/' && outPath.back() != '\\')
		outPath += '/';
	outPath += info.filename;

	return (info.archive->getType() == "FileSystem");
}

void Sample_MeshLod::addToProfile( Real cost )
{
	LodConfig config(mLodConfig);
	config.levels.clear();
	config.levels.push_back(mWorkLevel);
	ProgressiveMeshGenerator pm;
	pm.generateLodLevels(config);

	ProfiledEdge pv;
	if(pm._getLastVertexPos(pv.src)){
		pm._getLastVertexCollapseTo(pv.dst);
		// Prevent duplicates if you edit the same vertex twice.
		int size = mLodConfig.advanced.profile.size();
		for(int i=0;i<size;i++){
			ProfiledEdge& v = mLodConfig.advanced.profile[i];
			if(v.src == pv.src && v.dst == pv.dst){
				v.cost = cost;
				mProfileList->selectItem(i, false);
				loadUserLod();
				return;
			}
		}
		// Copy profile in queued build.
		pv.cost = cost;
		mLodConfig.advanced.profile.push_back(pv);
		mProfileList->addItem(StringConverter::toString(pv.src));
		mProfileList->selectItem(mProfileList->getNumItems() - 1, false);
	} else {
		mTrayMgr->showOkDialog("Error", "No vertex selected, because the mesh is not reduced.");
	}
	loadUserLod();
}

bool Sample_MeshLod::frameStarted( const FrameEvent& evt )
{
	mDistanceLabel->setCaption("Distance: " + StringConverter::toString(getCameraDistance()) + "px");
	return true;
}

void Sample_MeshLod::checkBoxToggled( CheckBox * box )
{
	if(box->getName() == "chkUseVertexNormals") {
		mLodConfig.advanced.useVertexNormals = box->isChecked();
		loadUserLod();
	} else if (box->getName() == "chkShowWireframe") {
		mCameraMan->getCamera()->setPolygonMode(mWireframe->isChecked() ? PM_WIREFRAME : PM_SOLID);
	}
}

void Sample_MeshLod::itemSelected( SelectMenu* menu )
{
	if (menu->getName() == "cmbModels") {
		changeSelectedMesh(menu->getSelectedItem());
	} else if(menu->getName() == "cmbLodLevels") {
		loadLodLevel(menu->getSelectionIndex());
	}
}

void Sample_MeshLod::sliderMoved(Slider* slider)
{
	if (slider->getName() == "sldReductionValue") {
		mWorkLevel.reductionValue = slider->getValue();
		loadUserLod();
	} else if (slider->getName() == "sldOutsideWeight") {
		mLodConfig.advanced.outsideWeight = (mOutsideWeightSlider->getValue() * mOutsideWeightSlider->getValue()) / 10000;
		loadUserLod();
	} else if (slider->getName() == "sldOutsideWalkAngle") {
		mLodConfig.advanced.outsideWalkAngle = mOutsideWalkAngle->getValue();
		loadUserLod();
	}
	
}

void Sample_MeshLod::buttonHit( Button* button )
{
	if(button->getName() == "btnReduceMore") {
		mReductionSlider->setValue(mReductionSlider->getValue()+1);
	} else if(button->getName() == "btnReduceLess") {
		mReductionSlider->setValue(mReductionSlider->getValue()-1);
	} else if(button->getName() == "btnAddToProfile") {
		addToProfile(std::numeric_limits<Real>::max());
	} else if(button->getName() == "btnRemoveFromProfile") {
		if(!mLodConfig.advanced.profile.empty()){
			LodProfile& profile = mLodConfig.advanced.profile;
			profile.erase(profile.begin() + mProfileList->getSelectionIndex());
			mProfileList->removeItem(mProfileList->getSelectionIndex());
		}
	} else if(button->getName() == "btnRemoveSelectedLodLevel") {
		removeLodLevel();
	} else if(button->getName() == "btnAddLodLevel") {
		addLodLevel();
	} else if(button->getName() == "btnAutoconfigure") {
		mTrayMgr->destroyAllWidgetsInTray(TL_TOP);
		mTrayMgr->createLabel(TL_TOP, "lblWhatYouSee", "Showing autoconfigured LOD", 300);
		loadAutomaticLod();
		forceLodLevel(-1); // disable Lod level forcing
	} else if (button->getName() == "btnShowAll") {
		loadUserLod(false);
		mTrayMgr->destroyAllWidgetsInTray(TL_TOP);
		mTrayMgr->createLabel(TL_TOP, "lblWhatYouSee", "Showing all LOD levels", 300);
		forceLodLevel(-1); // disable Lod level forcing
	} else if(button->getName() == "btnShowMesh") {
		mTrayMgr->destroyAllWidgetsInTray(TL_TOP);
		mTrayMgr->createLabel(TL_TOP, "lblWhatYouSee", "Showing LOD from mesh file", 300);
		if(mMeshEntity){
			mSceneMgr->destroyEntity(mMeshEntity);
			mMeshEntity = 0;
		}
		mLodConfig.mesh->reload();
		mMeshEntity = mSceneMgr->createEntity(mLodConfig.mesh->getName(), mLodConfig.mesh);
		mMeshNode->attachObject(mMeshEntity);
		forceLodLevel(-1); // disable Lod level forcing
		//String filename("");
		//getResourceFullPath(mLodConfig.mesh, filename);
		//mTrayMgr->showOkDialog("Success", "Showing mesh from: " + filename);
	} else if(button->getName() == "btnSaveMesh") {
		if(!mTrayMgr->getTrayContainer(TL_TOP)->isVisible() && !mLodConfig.levels.empty()){
			PMWorker::getSingleton().clearPendingLodRequests();
			ProgressiveMeshGenerator pm; // Non-threaded
			pm.generateLodLevels(mLodConfig);
			forceLodLevel(-1); // disable Lod level forcing
		}
		String filename("");
		if(!getResourceFullPath(mLodConfig.mesh, filename) || filename == "") {
			mTrayMgr->showOkDialog("Error", "'" + filename + "' is not a writable path!");
		} else {
			MeshSerializer ms;
			ms.exportMesh(mLodConfig.mesh.get(), filename);
			mTrayMgr->showOkDialog("Success", "Mesh saved to: " + filename);
		}
		if(!mTrayMgr->getTrayContainer(TL_TOP)->isVisible()){
			loadUserLod();
		}
	}
}

bool Sample_MeshLod::shouldInject( PMGenRequest* request )
{
	return true;
}

void Sample_MeshLod::injectionCompleted( PMGenRequest* request )
{
	forceLodLevel(mForcedLodLevel, false);
}
