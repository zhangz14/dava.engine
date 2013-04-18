//
//  HierarchyTreeAggregatorNode.cpp
//  UIEditor
//
//  Created by adebt on 3/11/13.
//
//

#include "HierarchyTreeAggregatorNode.h"
#include "LibraryController.h"
#include "ResourcesManageHelper.h"
#include "HierarchyTreePlatformNode.h"
#include "HierarchyTreeAggregatorControlNode.h"

#include "UI/UIList.h"
#include "EditorListDelegate.h"

#define WIDTH_NODE "width"
#define HEIGHT_NODE "height"

using namespace DAVA;

HierarchyTreeAggregatorNode::HierarchyTreeAggregatorNode(HierarchyTreePlatformNode* parent, const QString& name, const Rect& rect) :
	HierarchyTreeScreenNode(parent, name)
{
	this->rect = rect;
	screen->SetRect(rect);
	
	LibraryController::Instance()->AddControl(this);
}

HierarchyTreeAggregatorNode::HierarchyTreeAggregatorNode(HierarchyTreePlatformNode* parent,
														 const HierarchyTreeAggregatorNode* base)
:	HierarchyTreeScreenNode(parent, base)
{
	this->rect = base->GetRect();
	screen->SetRect(rect);

	LibraryController::Instance()->AddControl(this);
}

HierarchyTreeAggregatorNode::~HierarchyTreeAggregatorNode()
{
	LibraryController::Instance()->RemoveControl(this);
	DVASSERT(childs.size() == 0);
}

void HierarchyTreeAggregatorNode::AddChild(HierarchyTreeControlNode *node)
{
	childs.insert(node);
	UpdateChilds();
}

void HierarchyTreeAggregatorNode::RemoveChild(HierarchyTreeControlNode* node)
{
	CHILDS::iterator iter = childs.find(node);
	if (iter != childs.end())
		childs.erase(node);
}

void HierarchyTreeAggregatorNode::SetRect(const Rect& rect)
{
	this->rect = rect;
	screen->SetRect(rect);
	UpdateChilds();
}

Rect HierarchyTreeAggregatorNode::GetRect() const
{
	Rect rect = this->rect;
	CombineRectWithChild(rect);
	return rect;
}

void HierarchyTreeAggregatorNode::SetParent(HierarchyTreeNode* node, HierarchyTreeNode* insertAfter)
{
	Rect rect = GetScreen()->GetRect();
	HierarchyTreeScreenNode::SetParent(node, insertAfter);
	GetScreen()->SetRect(rect); //restore rect
}

HierarchyTreeControlNode* HierarchyTreeAggregatorNode::CreateChild(HierarchyTreeNode* parentNode, const QString& name)
{
	UIAggregatorControl* control = new UIAggregatorControl();
	control->CopyDataFrom(GetScreen());
	HierarchyTreeControlNode* controlNode = new HierarchyTreeAggregatorControlNode(this, parentNode, control, name);
	childs.insert(controlNode);
	UpdateChilds();
	return controlNode;
}

void HierarchyTreeAggregatorNode::UpdateChilds()
{
	for (CHILDS::iterator iter = childs.begin(); iter != childs.end(); ++iter)
	{
		HierarchyTreeControlNode* controlNode = (*iter);
		
		UIAggregatorControl* aggregatorControl = dynamic_cast<UIAggregatorControl*>(controlNode->GetUIObject());
		DVASSERT(aggregatorControl);
		if (!aggregatorControl)
			continue;
		
		// Remove any child controls of UIControl to prevent appearance of deleted
		// child in case when screen child is aggregator.
		/*List<UIControl*> aggregatorChilds = aggregatorControl->GetChildren();
		int size = GetScreen()->GetChildren().size();
		for (List<UIControl*>::iterator iter = aggregatorChilds.begin(); iter != aggregatorChilds.end();)
		{
			if (--size < 0)
				break;
			UIControl* child = (*iter);
			++iter;
			aggregatorControl->RemoveControl(child);
		}*/

		aggregatorControl->RemoveAllControls();
		
		const List<UIControl*> & childsList = screen->GetChildren();
		UIControl* belowControl = NULL;
		List<UIControl*>::const_iterator belowIter = aggregatorControl->GetChildren().begin();
		if (belowIter != aggregatorControl->GetChildren().end())
			belowControl = (*belowIter);
		for (List<UIControl*>::const_iterator iter = childsList.begin(); iter != childsList.end(); ++iter)
		{
			UIControl* control = (*iter);
			UIControl* newControl = control->Clone();
			aggregatorControl->InsertChildBelow(newControl, belowControl);
			aggregatorControl->AddAggregatorChild(newControl);
		}
		//aggregatorControl->SetSize(screen->GetSize()); TODO:// update child size 
		aggregatorControl->SetRect(aggregatorControl->GetRect());	//update childs size and position
	}
}

void HierarchyTreeAggregatorNode::RemoveSelection()
{
	UpdateChilds();
}

bool HierarchyTreeAggregatorNode::Load(const Rect& rect, const QString& path)
{
	this->path = ResourcesManageHelper::GetResourceRelativePath(path, true).toStdString();
	this->rect = rect;
	screen->SetRect(rect);

	bool result = HierarchyTreeScreenNode::Load(path);
	UpdateHierarchyTree();
	return result;
}

bool HierarchyTreeAggregatorNode::Save(YamlNode* node, const QString& path, bool saveAll)
{
	for (CHILDS::iterator iter = childs.begin(); iter != childs.end(); ++iter)
	{
		HierarchyTreeControlNode* controlNode = (*iter);
		
		UIAggregatorControl* aggregatorControl = dynamic_cast<UIAggregatorControl*>(controlNode->GetUIObject());
		DVASSERT(aggregatorControl);
		if (!aggregatorControl)
			continue;

		String aggregatorName, aggregatorPath;
		FileSystem::SplitPath(path.toStdString(), aggregatorPath, aggregatorName);
		aggregatorControl->SetAggregatorPath(aggregatorName);
	}
	
	node->Set(WIDTH_NODE, (int32)rect.dx);
	node->Set(HEIGHT_NODE, (int32)rect.dy);
	return HierarchyTreeScreenNode::Save(path, saveAll);
}

void HierarchyTreeAggregatorNode::UpdateHierarchyTree()
{
	const HierarchyTreeNode::HIERARCHYTREENODESLIST& child = parent->GetChildNodes();
	for (HierarchyTreeNode::HIERARCHYTREENODESLIST::const_iterator iter = child.begin();
		 iter != child.end();
		 ++iter)
	{
		HierarchyTreeScreenNode* node = dynamic_cast<HierarchyTreeScreenNode*>(*iter);
		DVASSERT(node);
		if (!node)
			continue;
		
		const HierarchyTreeNode::HIERARCHYTREENODESLIST& child = node->GetChildNodes();
		for (HierarchyTreeNode::HIERARCHYTREENODESLIST::const_iterator iter = child.begin();
			 iter != child.end();
			 ++iter)
		{
			HierarchyTreeControlNode* node = dynamic_cast<HierarchyTreeControlNode*>(*iter);
			DVASSERT(node);
			if (!node)
				continue;
			
			ReplaceAggregator(node);
		}
	}
}

void HierarchyTreeAggregatorNode::ReplaceAggregator(HierarchyTreeControlNode *node)
{
	UIList *list = dynamic_cast<UIList*>(node->GetUIObject());
	// For UIList control we should should always create a delegate
	// Set aggregator ID for list if it has saved aggregator path and it is available in tree
	if (list && list->GetAggregatorPath().compare(path) == 0)
	{
		EditorListDelegate *listDelegate = new EditorListDelegate(list->GetRect());
		// If loaded delegate has aggregator path - pass its id to delegate
		listDelegate->SetAggregatorID(GetId());
		// Always set a delegate for loaded UIList
		list->SetDelegate(listDelegate);
	}

	UIAggregatorControl* uiAggregator = dynamic_cast<UIAggregatorControl*>(node->GetUIObject());

	String aggregatorName, aggregatorPath;
	FileSystem::SplitPath(path, aggregatorPath, aggregatorName);

	if (uiAggregator && uiAggregator->GetAggregatorPath().compare(aggregatorName) == 0)
	{
		Logger::Debug(uiAggregator->GetAggregatorPath().c_str());
		HIERARCHYTREENODESLIST childs = node->GetChildNodes();
		uint32 i = 0;
		for (HIERARCHYTREENODESLIST::iterator iter = childs.begin(); iter != childs.end(); ++iter)
		{
			HierarchyTreeNode* childNode = (*iter);
			node->RemoveTreeNode(childNode, true, false);
			if (++i == GetScreen()->GetChildren().size())
				break;
		}
	
		HierarchyTreeAggregatorControlNode* controlNode = dynamic_cast<HierarchyTreeAggregatorControlNode*>(node);
		DVASSERT(controlNode);
		if (controlNode)
			controlNode->SetAggregatorNode(this);
		this->childs.insert(node);
		UpdateChilds();
	}
	
	const HierarchyTreeNode::HIERARCHYTREENODESLIST& child = node->GetChildNodes();
	for (HierarchyTreeNode::HIERARCHYTREENODESLIST::const_iterator iter = child.begin();
		 iter != child.end();
		 ++iter)
	{
		HierarchyTreeControlNode* node = dynamic_cast<HierarchyTreeControlNode*>(*iter);
		DVASSERT(node);
		if (!node)
			continue;
		
		ReplaceAggregator(node);
	}
}

void HierarchyTreeAggregatorNode::SetName(const QString& name)
{
	HierarchyTreeScreenNode::SetName(name);
	LibraryController::Instance()->UpdateControl(this);
}

const HierarchyTreeAggregatorNode::CHILDS& HierarchyTreeAggregatorNode::GetChilds() const
{
	return childs;
}

const String& HierarchyTreeAggregatorNode::GetPath()
{
	return path;
}