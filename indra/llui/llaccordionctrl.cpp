/** 
 * @file llaccordionctrl.cpp
 * @brief Accordion panel  implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#include "linden_common.h"

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"

#include "lluictrlfactory.h" // builds floaters from XML

#include "llwindow.h"
#include "llfocusmgr.h"
#include "lllocalcliprect.h"

#include "boost/bind/bind.hpp"

using namespace boost::placeholders;

static const S32 BORDER_MARGIN = 2;
static const S32 PARENT_BORDER_MARGIN = 5;
static const S32 VERTICAL_MULTIPLE = 16;
static const F32 MIN_AUTO_SCROLL_RATE = 120.f;
static const F32 MAX_AUTO_SCROLL_RATE = 500.f;
static const F32 AUTO_SCROLL_RATE_ACCEL = 120.f;

// LLAccordionCtrl =================================================================|

//static LLDefaultChildRegistry::Register<LLAccordionCtrl>	t2("accordion");
static LLRegisterWidget<LLAccordionCtrl> t2("accordion");

/*LLAccordionCtrl::LLAccordionCtrl(const Params& params):LLPanel(params)
 , mFitParent(params.fit_parent)
 , mAutoScrolling( false )
 , mAutoScrollRate( 0.f )
 , mSelectedTab( NULL )
 , mTabComparator( NULL )
 , mNoVisibleTabsHelpText(NULL)
 , mNoVisibleTabsOrigString(params.no_visible_tabs_text.initial_value().asString())
{
	initNoTabsWidget(params.no_matched_tabs_text);

	mSingleExpansion = params.single_expansion;
	if(mFitParent && !mSingleExpansion)
	{
		LL_INFOS() << "fit_parent works best when combined with single_expansion" << LL_ENDL;
	}
}*/

LLAccordionCtrl::LLAccordionCtrl() : LLPanel()
 , mAutoScrolling( false )
 , mAutoScrollRate( 0.f )
 , mSelectedTab( NULL )
 , mNoVisibleTabsHelpText(NULL)
{
	initNoTabsWidget(""/*LLTextBox::Params()*/);

	mSingleExpansion = false;
	mFitParent = false;
	LLUICtrlFactory::getInstance()->buildPanel(this, "accordion_parent.xml");
}

//---------------------------------------------------------------------------------
void LLAccordionCtrl::draw()
{
	if (mAutoScrolling)
	{
		// add acceleration to autoscroll
		mAutoScrollRate = llmin(mAutoScrollRate + (LLFrameTimer::getFrameDeltaTimeF32() * AUTO_SCROLL_RATE_ACCEL), MAX_AUTO_SCROLL_RATE);
	}
	else
	{
		// reset to minimum for next time
		mAutoScrollRate = MIN_AUTO_SCROLL_RATE;
	}
	// clear this flag to be set on next call to autoScroll
	mAutoScrolling = false;

	LLRect local_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
	
	LLLocalClipRect clip(local_rect);
	
	LLPanel::draw();
}


//---------------------------------------------------------------------------------
BOOL LLAccordionCtrl::postBuild()
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	LLRect scroll_rect;
	scroll_rect.setOriginAndSize( 
		getRect().getWidth() - scrollbar_size,
		1,
		scrollbar_size,
		getRect().getHeight() - 1);
	
	mScrollbar = new LLScrollbar("scrollable vertical", scroll_rect, LLScrollbar::VERTICAL, mInnerRect.getHeight(), 0, mInnerRect.getHeight(), boost::bind(&LLAccordionCtrl::onScrollPosChangeCallback, this, _1, _2), VERTICAL_MULTIPLE);
	mScrollbar->setFollows(FOLLOWS_RIGHT | FOLLOWS_TOP | FOLLOWS_BOTTOM);
	LLView::addChild( mScrollbar );
	mScrollbar->setVisible( false );
	mScrollbar->setFollows(FOLLOWS_RIGHT|FOLLOWS_TOP|FOLLOWS_BOTTOM);

	//if it was created from xml...
	std::vector<LLUICtrl*> accordion_tabs;
	for(child_list_const_iter_t it = getChildList()->begin(); 
		getChildList()->end() != it; ++it)
	{
		LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(*it);
		if(accordion_tab == NULL)
			continue;
		if(std::find(mAccordionTabs.begin(),mAccordionTabs.end(),accordion_tab) == mAccordionTabs.end())
		{
			accordion_tabs.push_back(accordion_tab);
		}
	}

	for(std::vector<LLUICtrl*>::reverse_iterator it = accordion_tabs.rbegin();it!=accordion_tabs.rend();++it)
		addCollapsibleCtrl(*it);

	arrange	();

	if(mSingleExpansion)
	{
		if(!mAccordionTabs[0]->getDisplayChildren())
			mAccordionTabs[0]->setDisplayChildren(true);
		for(size_t i=1;i<mAccordionTabs.size();++i)
		{
			if(mAccordionTabs[i]->getDisplayChildren())
				mAccordionTabs[i]->setDisplayChildren(false);
		}
	}

	updateNoTabsHelpTextVisibility();

	return TRUE;
}


//---------------------------------------------------------------------------------
LLAccordionCtrl::~LLAccordionCtrl()
{
  mAccordionTabs.clear();
}

//---------------------------------------------------------------------------------

void LLAccordionCtrl::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	// adjust our rectangle
	LLRect rcLocal = getRect();
	rcLocal.mRight = rcLocal.mLeft + width;
	rcLocal.mTop = rcLocal.mBottom + height;

	// get textbox a chance to reshape its content
	mNoVisibleTabsHelpText->reshape(width, height, called_from_parent);

	setRect(rcLocal);

	// assume that help text is always fit accordion.
	// necessary text paddings can be set via h_pad and v_pad
	mNoVisibleTabsHelpText->setRect(getLocalRect());

	arrange();
}

//---------------------------------------------------------------------------------
BOOL LLAccordionCtrl::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	return LLPanel::handleRightMouseDown(x, y, mask);
}

//---------------------------------------------------------------------------------
void LLAccordionCtrl::shiftAccordionTabs(S16 panel_num, S32 delta)
{
	for(size_t i = panel_num; i < mAccordionTabs.size(); i++ )
	{
		ctrlShiftVertical(mAccordionTabs[i],delta);
	}	
}


//---------------------------------------------------------------------------------
void LLAccordionCtrl::onCollapseCtrlCloseOpen(S16 panel_num) 
{
	if(mSingleExpansion)
	{
		for(size_t i=0;i<mAccordionTabs.size();++i)
		{
			if(i==panel_num)
				continue;
			if(mAccordionTabs[i]->getDisplayChildren())
				mAccordionTabs[i]->setDisplayChildren(false);
		}

	}
	arrange();
}

void LLAccordionCtrl::show_hide_scrollbar(S32 width, S32 height)
{
	calcRecuiredHeight();
	if(getRecuiredHeight() > height )
		showScrollbar(width,height);
	else
		hideScrollbar(width,height);
}

void	LLAccordionCtrl::showScrollbar(S32 width, S32 height)
{
	bool was_visible = mScrollbar->getVisible();

	mScrollbar->setVisible(true);
	
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	ctrlSetLeftTopAndSize(mScrollbar
		,width-scrollbar_size - PARENT_BORDER_MARGIN/2
		,height-PARENT_BORDER_MARGIN
		,scrollbar_size
		,height-2*PARENT_BORDER_MARGIN);
	
	mScrollbar->setPageSize(height);
	mScrollbar->setDocParams(mInnerRect.getHeight(),mScrollbar->getDocPos());

	if(was_visible)
	{
		S32 scroll_pos = llmin(mScrollbar->getDocPos(), getRecuiredHeight() - height - 1);
		mScrollbar->setDocPos(scroll_pos);
	}
}

void	LLAccordionCtrl::hideScrollbar( S32 width, S32 height )
{
	if(mScrollbar->getVisible() == false)
		return;
	mScrollbar->setVisible(false);

	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	S32 panel_width = width - 2*BORDER_MARGIN;

	//reshape all accordeons and shift all draggers
	for(size_t i=0;i<mAccordionTabs.size();++i)
	{
		LLRect panel_rect = mAccordionTabs[i]->getRect();
		ctrlSetLeftTopAndSize(mAccordionTabs[i],panel_rect.mLeft,panel_rect.mTop,panel_width,panel_rect.getHeight());
	}

	mScrollbar->setDocPos(0);

	if(mAccordionTabs.size()>0)
	{
		S32 panel_top = height - BORDER_MARGIN;		  // Top coordinate of the first panel
		S32 diff = panel_top - mAccordionTabs[0]->getRect().mTop;
		shiftAccordionTabs(0,diff);
	}
}


//---------------------------------------------------------------------------------
S32 LLAccordionCtrl::calcRecuiredHeight()
{
	S32 rec_height = 0;
	
	std::vector<LLAccordionCtrlTab*>::iterator panel;
	for(panel=mAccordionTabs.begin(); panel!=mAccordionTabs.end(); ++panel)
	{
		LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(*panel);
		if(accordion_tab && accordion_tab->getVisible())
		{
			rec_height += accordion_tab->getRect().getHeight();
		}
	}

	mInnerRect.setLeftTopAndSize(0,rec_height + BORDER_MARGIN*2,getRect().getWidth(),rec_height + BORDER_MARGIN);

	return mInnerRect.getHeight();
}

//---------------------------------------------------------------------------------
void LLAccordionCtrl::ctrlSetLeftTopAndSize(LLView* panel, S32 left, S32 top, S32 width, S32 height)
{
	if(!panel)
		return;
	LLRect panel_rect = panel->getRect();
	panel_rect.setLeftTopAndSize( left, top, width, height);
	panel->reshape( width, height, 1);
	panel->setRect(panel_rect);
}

void LLAccordionCtrl::ctrlShiftVertical(LLView* panel,S32 delta)
{
	if(!panel)
		return;
	panel->translate(0,delta);
}

//---------------------------------------------------------------------------------

void LLAccordionCtrl::addCollapsibleCtrl(LLView* view)
{
	LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(view);
	if(!accordion_tab)
		return;
	if(std::find(beginChild(), endChild(), accordion_tab) == endChild())
		addChild(accordion_tab);
	mAccordionTabs.push_back(accordion_tab);

	accordion_tab->setDropDownStateChangedCallback( boost::bind(&LLAccordionCtrl::onCollapseCtrlCloseOpen, this, S16(mAccordionTabs.size() - 1)) );
	arrange();	
}

void LLAccordionCtrl::removeCollapsibleCtrl(LLView* view)
{
	LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(view);
	if(!accordion_tab)
		return;

	if(std::find(beginChild(), endChild(), accordion_tab) != endChild())
		removeChild(accordion_tab);

	for (std::vector<LLAccordionCtrlTab*>::iterator iter = mAccordionTabs.begin();
			iter != mAccordionTabs.end(); ++iter)
	{
		if (accordion_tab == (*iter))
		{
			mAccordionTabs.erase(iter);
			break;
		}
	}

	// if removed is selected - reset selection
	if (mSelectedTab == view)
	{
		mSelectedTab = NULL;
	}
}

void	LLAccordionCtrl::initNoTabsWidget(const std::string/*LLTextBox::Params*/& tb_params)
{
	//LLTextBox::Params tp = tb_params;
	//tp.rect(getLocalRect());
	mNoMatchedTabsOrigString = tb_params; //tp.initial_value().asString();
	mNoVisibleTabsHelpText = new LLTextBox(tb_params, getLocalRect()); //LLUICtrlFactory::create<LLTextBox>(tp, this);
}

void	LLAccordionCtrl::updateNoTabsHelpTextVisibility()
{
	bool visible_exists = false;
	std::vector<LLAccordionCtrlTab*>::const_iterator it = mAccordionTabs.begin();
	const std::vector<LLAccordionCtrlTab*>::const_iterator it_end = mAccordionTabs.end();
	for (; it != it_end; ++it)
	{
		if ((*it)->getVisible())
		{
			visible_exists = true;
			break;
		}
	}

	mNoVisibleTabsHelpText->setVisible(!visible_exists);
}

void	LLAccordionCtrl::arrangeSinge()
{
	S32 panel_left = BORDER_MARGIN;	  // Margin from left side of Splitter
	S32 panel_top = getRect().getHeight() - BORDER_MARGIN;		  // Top coordinate of the first panel
	S32 panel_width = getRect().getWidth() - 4;		  // Top coordinate of the first panel
	S32 panel_height;

	S32 collapsed_height = 0;

	for(size_t i=0;i<mAccordionTabs.size();++i)
	{
		LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(mAccordionTabs[i]);
		
		if(accordion_tab->getVisible() == false) //skip hidden accordion tabs
			continue;
		if(!accordion_tab->isExpanded() )
		{
			collapsed_height+=mAccordionTabs[i]->getRect().getHeight();
		}
	}

	S32 expanded_height = getRect().getHeight() - BORDER_MARGIN - collapsed_height;
	
	for(size_t i=0;i<mAccordionTabs.size();++i)
	{
		LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(mAccordionTabs[i]);
		
		if(accordion_tab->getVisible() == false) //skip hidden accordion tabs
			continue;
		if(!accordion_tab->isExpanded() )
		{
			panel_height = accordion_tab->getRect().getHeight();
		}
		else
		{
			if(mFitParent)
			{
				panel_height = expanded_height;
			}
			else
			{
				if(accordion_tab->getAccordionView())
				{
					panel_height = accordion_tab->getAccordionView()->getRect().getHeight() + 
						accordion_tab->getHeaderHeight() + 2*BORDER_MARGIN;
				}
				else
				{
					panel_height = accordion_tab->getRect().getHeight();
				}
			}
		}

		// make sure at least header is shown
		panel_height = llmax(panel_height, accordion_tab->getHeaderHeight());

		ctrlSetLeftTopAndSize(mAccordionTabs[i], panel_left, panel_top, panel_width, panel_height);
		panel_top-=mAccordionTabs[i]->getRect().getHeight();
	}

	show_hide_scrollbar(getRect().getWidth(), getRect().getHeight());
	updateLayout(getRect().getWidth(), getRect().getHeight());
}

void	LLAccordionCtrl::arrangeMultiple()
{
	S32 panel_left = BORDER_MARGIN;	  // Margin from left side of Splitter
	S32 panel_top = getRect().getHeight() - BORDER_MARGIN;		  // Top coordinate of the first panel
	S32 panel_width = getRect().getWidth() - 4;		  // Top coordinate of the first panel

	//Calculate params	
	for(size_t i = 0; i < mAccordionTabs.size(); i++ )
	{
		LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(mAccordionTabs[i]);
		
		if(accordion_tab->getVisible() == false) //skip hidden accordion tabs
			continue;
		
		if(!accordion_tab->isExpanded() )
		{
			ctrlSetLeftTopAndSize(mAccordionTabs[i], panel_left, panel_top, panel_width, accordion_tab->getRect().getHeight());
			panel_top-=mAccordionTabs[i]->getRect().getHeight();
		}
		else
		{
			S32 panel_height = accordion_tab->getRect().getHeight();
			
			if(mFitParent)
			{
				// all expanded tabs will have equal height
				panel_height = calcExpandedTabHeight(i, panel_top);
				ctrlSetLeftTopAndSize(accordion_tab, panel_left, panel_top, panel_width, panel_height);

				// try to make accordion tab fit accordion view height.
				// Accordion View should implement getRequiredRect() and provide valid height
				S32 optimal_height = accordion_tab->getAccordionView()->getRequiredRect().getHeight();
				optimal_height += accordion_tab->getHeaderHeight() + 2 * BORDER_MARGIN;
				if(optimal_height < panel_height)
				{
					panel_height = optimal_height;
				}

				// minimum tab height is equal to header height
				if(mAccordionTabs[i]->getHeaderHeight() > panel_height)
				{
					panel_height = mAccordionTabs[i]->getHeaderHeight();
				}
			}
			
			ctrlSetLeftTopAndSize(mAccordionTabs[i], panel_left, panel_top, panel_width, panel_height);
			panel_top-=panel_height;
			
		}
	}	

	show_hide_scrollbar(getRect().getWidth(),getRect().getHeight());

	updateLayout(getRect().getWidth(),getRect().getHeight());
}


void LLAccordionCtrl::arrange()
{
	updateNoTabsHelpTextVisibility();

	if( mAccordionTabs.size() == 0)
	{
		//We do not arrange if we do not have what should be arranged
		return;
	}


	if(mAccordionTabs.size() == 1)
	{
		S32 panel_top = getRect().getHeight() - BORDER_MARGIN;		  // Top coordinate of the first panel
		S32 panel_width = getRect().getWidth() - 4;		  // Top coordinate of the first panel
		
		LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(mAccordionTabs[0]);
		
		LLRect panel_rect = accordion_tab->getRect();
		
		S32 panel_height = getRect().getHeight() - 2*BORDER_MARGIN;

		if (accordion_tab->getFitParent())
			panel_height = accordion_tab->getRect().getHeight();
		ctrlSetLeftTopAndSize(accordion_tab,panel_rect.mLeft,panel_top,panel_width,panel_height);
		
		show_hide_scrollbar(getRect().getWidth(),getRect().getHeight());
		return;

	}

	if(mSingleExpansion)
		arrangeSinge ();
	else
		arrangeMultiple ();
}

//---------------------------------------------------------------------------------

BOOL LLAccordionCtrl::handleScrollWheel		( S32 x, S32 y, S32 clicks )
{
	if(LLPanel::handleScrollWheel(x,y,clicks))
		return TRUE;
	if( mScrollbar->getVisible() && mScrollbar->handleScrollWheel( 0, 0, clicks ) )
		return TRUE;
	return false;

}

BOOL LLAccordionCtrl::handleKeyHere			(KEY key, MASK mask)
{
	if( mScrollbar->getVisible() && mScrollbar->handleKeyHere( key,mask ) )
		return TRUE;
	return LLPanel::handleKeyHere(key,mask);
}

BOOL LLAccordionCtrl::handleDragAndDrop		(S32 x, S32 y, MASK mask,
											 BOOL drop,
											 EDragAndDropType cargo_type,
											 void* cargo_data,
											 EAcceptance* accept,
											 std::string& tooltip_msg)
{
	// Scroll folder view if needed.  Never accepts a drag or drop.
	*accept = ACCEPT_NO;
	BOOL handled = autoScroll(x, y);

	if( !handled )
	{
		handled = childrenHandleDragAndDrop(x, y, mask, drop, cargo_type,
											cargo_data, accept, tooltip_msg) != NULL;
	}
	return TRUE;
}

BOOL LLAccordionCtrl::autoScroll		(S32 x, S32 y)
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	bool scrolling = false;
	if( mScrollbar->getVisible() )
	{
		LLRect rect_local( 0, getRect().getHeight(), getRect().getWidth() - scrollbar_size, 0 );
		LLRect screen_local_extents;

		// clip rect against root view
		screenRectToLocal(getRootView()->getLocalRect(), &screen_local_extents);
		rect_local.intersectWith(screen_local_extents);

		// autoscroll region should take up no more than one third of visible scroller area
		S32 auto_scroll_region_height = llmin(rect_local.getHeight() / 3, 10);
		S32 auto_scroll_speed = ll_pos_round(mAutoScrollRate * LLFrameTimer::getFrameDeltaTimeF32());

		LLRect bottom_scroll_rect = screen_local_extents;
		bottom_scroll_rect.mTop = rect_local.mBottom + auto_scroll_region_height;
		if( bottom_scroll_rect.pointInRect( x, y ) && (mScrollbar->getDocPos() < mScrollbar->getDocPosMax()) )
		{
			mScrollbar->setDocPos( mScrollbar->getDocPos() + auto_scroll_speed );
			mAutoScrolling = true;
			scrolling = true;
		}

		LLRect top_scroll_rect = screen_local_extents;
		top_scroll_rect.mBottom = rect_local.mTop - auto_scroll_region_height;
		if( top_scroll_rect.pointInRect( x, y ) && (mScrollbar->getDocPos() > 0) )
		{
			mScrollbar->setDocPos( mScrollbar->getDocPos() - auto_scroll_speed );
			mAutoScrolling = true;
			scrolling = true;
		}
	}
	return scrolling;
}

void	LLAccordionCtrl::updateLayout	(S32 width, S32 height)
{
	S32 panel_top = height - BORDER_MARGIN ;
	if(mScrollbar->getVisible())
		panel_top+=mScrollbar->getDocPos();

	S32 panel_width = width - 2*BORDER_MARGIN;

	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
	if(mScrollbar->getVisible())
		panel_width-=scrollbar_size;

	//set sizes for first panels and dragbars
	for(size_t i=0;i<mAccordionTabs.size();++i)
	{
		if(!mAccordionTabs[i]->getVisible())
			continue;
		LLRect panel_rect = mAccordionTabs[i]->getRect();
		ctrlSetLeftTopAndSize(mAccordionTabs[i],panel_rect.mLeft,panel_top,panel_width,panel_rect.getHeight());
		panel_top-=panel_rect.getHeight();
	}
}

void	LLAccordionCtrl::onScrollPosChangeCallback(S32, LLScrollbar*)
{
	updateLayout(getRect().getWidth(),getRect().getHeight());
}
/*void	LLAccordionCtrl::onOpen		(const LLSD& key)
{
	for(size_t i=0;i<mAccordionTabs.size();++i)
	{
		LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(mAccordionTabs[i]);
		LLPanel* panel = dynamic_cast<LLPanel*>(accordion_tab->getAccordionView());
		if(panel!=NULL)
		{
			panel->onOpen(key);
		}
	}
}*/
S32	LLAccordionCtrl::notifyParent(const LLSD& info)
{
	if(info.has("action"))
	{
		std::string str_action = info["action"];
		if(str_action == "size_changes")
		{
			//
			arrange();
			return 1;
		}
		else if(str_action == "select_next")
		{
			for(size_t i=0;i<mAccordionTabs.size();++i)
			{
				LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(mAccordionTabs[i]);
				if(accordion_tab->hasFocus())
				{
					while(++i<mAccordionTabs.size())
					{
						if(mAccordionTabs[i]->getVisible())
							break;
					}
					if(i<mAccordionTabs.size())
					{
						accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(mAccordionTabs[i]);
						accordion_tab->notify(LLSD().with("action","select_first"));
						return 1;
					}
					break;
				}
			}
			return 0;
		}
		else if(str_action == "select_prev")
		{
			for(size_t i=0;i<mAccordionTabs.size();++i)
			{
				LLAccordionCtrlTab* accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(mAccordionTabs[i]);
				if(accordion_tab->hasFocus() && i>0)
				{
					bool prev_visible_tab_found = false;
					while(i>0)
					{
						if(mAccordionTabs[--i]->getVisible())
						{
							prev_visible_tab_found = true;
							break;
						}
					}

					if (prev_visible_tab_found)
					{
						accordion_tab = dynamic_cast<LLAccordionCtrlTab*>(mAccordionTabs[i]);
						accordion_tab->notify(LLSD().with("action","select_last"));
						return 1;
					}
					break;
				}
			}
			return 0;
		}
		else if(str_action == "select_current")
		{
			for(size_t i=0;i<mAccordionTabs.size();++i)
			{
				// Set selection to the currently focused tab.
				if(mAccordionTabs[i]->hasFocus())
				{
					if (mAccordionTabs[i] != mSelectedTab)
					{
						if (mSelectedTab)
						{
							mSelectedTab->setSelected(false);
						}
						mSelectedTab = mAccordionTabs[i];
						mSelectedTab->setSelected(true);
					}

					return 1;
				}
			}
			return 0;
		}
		else if(str_action == "deselect_current")
		{
			// Reset selection to the currently selected tab.
			if (mSelectedTab)
			{
				mSelectedTab->setSelected(false);
				mSelectedTab = NULL;
				return 1;
			}
			return 0;
		}
	}
	else if (info.has("scrollToShowRect"))
	{
		LLRect screen_rc, local_rc;
		screen_rc.setValue(info["scrollToShowRect"]);
		screenRectToLocal(screen_rc, &local_rc);

		// Translate to parent coordinatess to check if we are in visible rectangle
		local_rc.translate( getRect().mLeft, getRect().mBottom );

		if ( !getRect().contains (local_rc) )
		{
			// Back to local coords and calculate position for scroller
			S32 bottom = mScrollbar->getDocPos() - local_rc.mBottom + getRect().mBottom;
			S32 top = mScrollbar->getDocPos() - local_rc.mTop + getRect().mTop;

			S32 scroll_pos = llclamp(mScrollbar->getDocPos(),
									 bottom, // min vertical scroll
									 top); // max vertical scroll 

			mScrollbar->setDocPos( scroll_pos );
		}
		return 1;
	}
	else if (info.has("child_visibility_change"))
	{
		BOOL new_visibility = info["child_visibility_change"];
		if (new_visibility)
		{
			// there is at least one visible tab
			mNoVisibleTabsHelpText->setVisible(FALSE);
		}
		else
		{
			// it could be the latest visible tab, check all of them
			updateNoTabsHelpTextVisibility();
		}
	}
	return LLPanel::notifyParent(info);
}
void	LLAccordionCtrl::reset		()
{
	if(mScrollbar)
		mScrollbar->setDocPos(0);
}

void LLAccordionCtrl::expandDefaultTab()
{
	if (mAccordionTabs.size() > 0)
	{
		LLAccordionCtrlTab* tab = mAccordionTabs.front();

		if (!tab->getDisplayChildren())
		{
			tab->setDisplayChildren(true);
		}

		for (size_t i = 1; i < mAccordionTabs.size(); ++i)
		{
			tab = mAccordionTabs[i];

			if (tab->getDisplayChildren())
			{
				tab->setDisplayChildren(false);
			}
		}

		arrange();
	}
}

void LLAccordionCtrl::sort()
{
	if (!mTabComparator)
	{
		LL_WARNS() << "No comparator specified for sorting accordion tabs." << LL_ENDL;
		return;
	}

	std::sort(mAccordionTabs.begin(), mAccordionTabs.end(), LLComparatorAdaptor(*mTabComparator));
	arrange();
}

void	LLAccordionCtrl::setFilterSubString(const std::string& filter_string)
{
	LLStringUtil::format_map_t args;
	args["[SEARCH_TERM]"] = LLURI::escape(filter_string);
	std::string text = filter_string.empty() ? mNoVisibleTabsOrigString : mNoMatchedTabsOrigString;
	LLStringUtil::format(text, args);

	mNoVisibleTabsHelpText->setValue(text);
}

const LLAccordionCtrlTab* LLAccordionCtrl::getExpandedTab() const
{
	typedef std::vector<LLAccordionCtrlTab*>::const_iterator tabs_const_iterator;

	const LLAccordionCtrlTab* result = 0;

	for (tabs_const_iterator i = mAccordionTabs.begin(); i != mAccordionTabs.end(); ++i)
	{
		if ((*i)->isExpanded())
		{
			result = *i;
			break;
		}
	}

	return result;
}

S32 LLAccordionCtrl::calcExpandedTabHeight(S32 tab_index /* = 0 */, S32 available_height /* = 0 */)
{
	if(tab_index < 0)
	{
		return available_height;
	}

	S32 collapsed_tabs_height = 0;
	S32 num_expanded = 0;

	for(size_t n = tab_index; n < mAccordionTabs.size(); ++n)
	{
		if(!mAccordionTabs[n]->isExpanded())
		{
			collapsed_tabs_height += mAccordionTabs[n]->getHeaderHeight();
		}
		else
		{
			++num_expanded;
		}
	}

	if(0 == num_expanded)
	{
		return available_height;
	}

	S32 expanded_tab_height = available_height - collapsed_tabs_height - BORDER_MARGIN; // top BORDER_MARGIN is added in arrange(), here we add bottom BORDER_MARGIN
	expanded_tab_height /= num_expanded;
	return expanded_tab_height;
}


//static 
LLView* LLAccordionCtrl::fromXML(LLXMLNodePtr node, LLView* parent, LLUICtrlFactory* factory)
{
	LLAccordionCtrl* ctrl = new LLAccordionCtrl();
	ctrl->mCommitCallbackRegistrar.pushScope();
	ctrl->mEnableCallbackRegistrar.pushScope();
	ctrl->initPanelXML(node, parent, factory);
	ctrl->mCommitCallbackRegistrar.popScope();
	ctrl->mEnableCallbackRegistrar.popScope();
	return ctrl;
}

void LLAccordionCtrl::initFromXML(LLXMLNodePtr node, LLView* parent)
{
	if (node->hasAttribute("single_expansion"))
		node->getAttribute_bool("single_expansion", mSingleExpansion);
	if (node->hasAttribute("fit_parent"))
		node->getAttribute_bool("fit_parent", mFitParent);
	if (node->hasAttribute("no_matched_tabs_text"))
	{
		node->getAttributeString("no_matched_tabs_text", mNoMatchedTabsOrigString);
		mNoVisibleTabsHelpText->setValue(mNoMatchedTabsOrigString);
	}
	if (node->hasAttribute("no_visible_tabs_text"))
		node->getAttributeString("no_visible_tabs_text", mNoVisibleTabsOrigString);

	LLPanel::initFromXML(node, parent);
}
