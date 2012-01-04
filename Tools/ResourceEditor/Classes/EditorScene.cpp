/*
 *  EditorScene.cpp
 *  SceneEditor
 *
 *  Created by Yury Danilov on 14.12.11
 *  Copyright 2011 DAVA. All rights reserved.
 *
 */

#include "EditorScene.h"
#include "SceneNodeUserData.h"

/*
    This means that if we'll call GameScene->GetClassName() it'll return "Scene"
    This is for correct serialization of framework subclasses.
 */
REGISTER_CLASS_WITH_ALIAS(EditorScene, "Scene");

EditorScene::EditorScene()
{ 
	selection = 0;
//	dynCollisionConfiguration = new btDefaultCollisionConfiguration();
//	dynDispatcher = new	btCollisionDispatcher(dynCollisionConfiguration);
    btVector3 worldMin(-1000,-1000,-1000);
	btVector3 worldMax(1000,1000,1000);
//	dynOverlappingPairCache = new btAxisSweep3(worldMin,worldMax);
//	btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
//	dynSolver = sol;
//	dynamicsWorld = new btDiscreteDynamicsWorld(dynDispatcher, dynOverlappingPairCache, dynSolver
//                                                , dynCollisionConfiguration);
//	dynamicsWorld->setGravity(btVector3(0,0,-9.81));
        
    collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	broadphase = new btAxisSweep3(worldMin,worldMax);
	collisionWorld = new btCollisionWorld(dispatcher, broadphase, collisionConfiguration);
}

EditorScene::~EditorScene()
{

}

void EditorScene::Update(float32 timeElapsed)
{    
    Scene::Update(timeElapsed);
//	depth = 0;
	CheckNodes(this);
	collisionWorld->updateAabbs();
//	Logger::Debug("CheckNodes end");
}

void EditorScene::CheckNodes(SceneNode * curr)
{
//	depth++;
//	Logger::Debug("%d CheckNodes curr: %s", depth, curr->GetName().c_str());

	MeshInstanceNode * mesh = dynamic_cast<MeshInstanceNode *> (curr);
	
	if (mesh && mesh->userData == 0)
	{
		SceneNodeUserData * data = new SceneNodeUserData();
		curr->userData = data;
		data->bulletObject = new BulletObject(this, collisionWorld, (MeshInstanceNode*)mesh, mesh->GetWorldTransform());
//		mesh->AddNode(data->bulletObject->GetDebugNode());
//		BulletLink link;
//		link.bulletObj = data->bulletObject;
//		link.sceneNode = curr;
//		links.push_back(link);
//		Logger::Debug("%d Meshes count: %d", depth, mesh->GetMeshes().size());
	}
	else if (mesh && mesh->userData)
	{
		SceneNodeUserData * data = (SceneNodeUserData*)curr->userData;
		data->bulletObject->UpdateCollisionObject();
	}

	int size = curr->GetChildrenCount();
//	Logger::Debug("GetChildrenCount %d",size);
	for (int i = 0; i < size; i++)
	{
		CheckNodes(curr->GetChild(i));
	}
}

void EditorScene::TrySelection(Vector3 from, Vector3 direction)
{
	if (selection)
		selection->SetDebugFlags(SceneNode::DEBUG_DRAW_NONE);

	btVector3 pos(from.x, from.y, from.z);
    btVector3 to(direction.x, direction.y, direction.z);
		
    btCollisionWorld::AllHitsRayResultCallback cb(pos, to);
    collisionWorld->rayTest(pos, to, cb);
	btCollisionObject * coll = 0;
	if (cb.hasHit()) 
    {
		Logger::Debug("Has Hit");
		int findedIndex = 0;
		if(selection)
		{
			SceneNodeUserData * data = (SceneNodeUserData*)selection->userData;
			for (int i = 0; i < cb.m_collisionObjects.size(); i++)
			{					
				if (data->bulletObject->GetCollisionObject() == cb.m_collisionObjects[i])
				{
					findedIndex = i;
					break;
				}
			}
			while (findedIndex < cb.m_collisionObjects.size() && data->bulletObject->GetCollisionObject() == cb.m_collisionObjects[findedIndex])
				findedIndex++;
			findedIndex = findedIndex % cb.m_collisionObjects.size();
		}
		Logger::Debug("size:%d selIndex:%d", cb.m_collisionObjects.size(), findedIndex);
		
		coll = cb.m_collisionObjects[findedIndex];
		selection = FindSelected(this, coll);
	
		if(selection)
			selection->SetDebugFlags(SceneNode::DEBUG_DRAW_AABOX_CORNERS);
	}
	else 
	{
		selection = 0;		
	}
}

SceneNode * EditorScene::FindSelected(SceneNode * curr, btCollisionObject * coll)
{
	MeshInstanceNode * mesh = dynamic_cast<MeshInstanceNode *> (curr);
	
	if (mesh && mesh->userData)
	{
		SceneNodeUserData * data = (SceneNodeUserData*)curr->userData;
		if (data->bulletObject->GetCollisionObject() == coll)
			return curr;
	}
	int size = curr->GetChildrenCount();
	for (int i = 0; i < size; i++)
	{
		SceneNode * result = FindSelected(curr->GetChild(i), coll);
		if (result)
			return result;
	}
	return 0;
}

SceneNode * EditorScene::GetSelection()
{
	return selection;
}


