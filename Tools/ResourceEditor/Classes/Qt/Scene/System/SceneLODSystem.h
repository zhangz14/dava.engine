/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/



#ifndef __SCENE_LOD_SYSTEM_H__
#define __SCENE_LOD_SYSTEM_H__

// framework
#include "Entity/SceneSystem.h"
#include "DAVAEngine.h"

class EntityGroup;
class Command2;

class SceneLODSystem : public DAVA::SceneSystem
{
	friend class SceneEditor2;
	friend class EntityModificationSystem;

public:
	SceneLODSystem(DAVA::Scene * scene);
	~SceneLODSystem();
	DAVA::uint32 GetLayersCount() const;
	DAVA::float32 GetLayerDistance(DAVA::uint32 layerNum) const;
	void SetLayerDistance(DAVA::uint32 layerNum, DAVA::float32 distance);

	void UpdateDistances(const DAVA::Map<DAVA::uint32, DAVA::float32> & lodDistances);

	DAVA::uint32 GetLayerTriangles(DAVA::uint32 layerNum) const;

	void EnableForceDistance(bool enable);
	bool GetForceDistanceEnabled() const;

	void SetForceDistance(DAVA::float32 distance);
	DAVA::float32 GetForceDistance() const;

	void SetForceLayer(DAVA::int32 layer);
	DAVA::int32 GetForceLayer() const;

	void GetLODDataFromScene();

	void CreatePlaneLOD(DAVA::int32 fromLayer, DAVA::uint32 textureSize, const DAVA::FilePath & texturePath);
	bool CanCreatePlaneLOD();
	DAVA::FilePath GetDefaultTexturePathForPlaneEntity();

	static void EnumerateLODsRecursive(DAVA::Entity *entity, DAVA::Vector<DAVA::LodComponent *> & lods);
	static void AddTrianglesInfo(DAVA::uint32 triangles[], DAVA::LodComponent *lod, bool onlyVisibleBatches);

	//TODO: remove after lod editing implementation
	DAVA_DEPRECATED(void CopyLastLodToLod0());

	bool CanDeleteLod();

	void EnableAllSceneMode(bool enabled);

	void DeleteFirstLOD();
	void DeleteLastLOD();

	void SceneStructureChanged(SceneEditor2 *scene, DAVA::Entity *parent);
	void SceneSelectionChanged(SceneEditor2 *scene, const EntityGroup *selected, const EntityGroup *deselected);
protected:

	void ClearLODData();
	void ClearForceData();
	void UpdateForceData();
	void EnumerateLODs();
	void ResetForceState(DAVA::Entity *entity);


protected:

	DAVA::uint32 lodLayersCount;
	DAVA::float32 lodDistances[DAVA::LodComponent::MAX_LOD_LAYERS];
	DAVA::uint32 lodTriangles[DAVA::LodComponent::MAX_LOD_LAYERS];


	bool forceDistanceEnabled;
	DAVA::float32 forceDistance;
	DAVA::int32 forceLayer;

	DAVA::Vector<DAVA::LodComponent *> lodData;
	SceneEditor2 *scene2;
	bool allSceneModeEnabled;
protected:

protected:
	DAVA::Set<DAVA::Entity*> entities;
};

#endif // __SCENE_LOD_SYSTEM_H__
