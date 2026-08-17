/*
    Copyright 2021 XITRIX

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <borealis/core/application.hpp>
#include <borealis/core/touch/tap_gesture.hpp>
#include <borealis/views/recycler.hpp>

namespace brls
{

RecyclerCell::RecyclerCell()
{
    this->setLineBottom(1);
    this->setLineColor(Application::getTheme()["brls/sidebar/separator"]);

    setHeight(Application::getStyle()["brls/dropdown/listItemHeight"]);

    this->registerClickAction([this](View* view) {
        RecyclerFrame* recycler = dynamic_cast<RecyclerFrame*>(getParent()->getParent());
        if (recycler)
            recycler->getDataSource()->didSelectRowAt(recycler, indexPath);
        return true;
    });

    subscription = Application::getGlobalInputTypeChangeEvent()->subscribe([this](InputType type) {
        bool isTouch = type == InputType::TOUCH;
        this->setLineColor((!isTouch && this->focused) ? TRANSPARENT : Application::getTheme()["brls/sidebar/separator"]);
    });

    this->addGestureRecognizer(new TapGestureRecognizer(this));
}

RecyclerCell::~RecyclerCell()
{
    Application::getGlobalInputTypeChangeEvent()->unsubscribe(subscription);
}

RecyclerCell* RecyclerCell::create()
{
    return new RecyclerCell();
}

void RecyclerCell::setIndexPath(IndexPath value)
{
    indexPath = value;

    this->setLineTop(value.row == 0 ? 1 : 0);
}

void RecyclerCell::onFocusGained()
{
    // Called when a child of ours gets focused, in that case it's the Image

    Box::onFocusGained();

    bool isTouch = Application::getInputType() == InputType::TOUCH;
    this->setLineColor(!isTouch ? TRANSPARENT : Application::getTheme()["brls/sidebar/separator"]);
}

void RecyclerCell::onFocusLost()
{
    // Called when a child of ours losts focused, in that case it's the Image

    Box::onFocusLost();
    this->setLineColor(Application::getTheme()["brls/sidebar/separator"]);
}

RecyclerHeader::RecyclerHeader()
{
    this->header = new Header();
    this->addView(header);
    header->setGrow(1);
}

void RecyclerHeader::setTitle(std::string title)
{
    this->header->setTitle(title);
}

void RecyclerHeader::setSubtitle(std::string subtitle)
{
    this->header->setSubtitle(subtitle);
}

RecyclerHeader* RecyclerHeader::create()
{
    return new RecyclerHeader();
}

RecyclerCell* RecyclerDataSource::cellForHeader(RecyclerFrame* recycler, int section)
{
    RecyclerHeader* header = (RecyclerHeader*)recycler->dequeueReusableCell("brls::Header");
    std::string title      = this->titleForHeader(recycler, section);
    header->setTitle(title);
    header->setVisibility(title.empty() ? Visibility::GONE : Visibility::VISIBLE);
    header->setHeight(title.empty() ? 0 : View::AUTO);
    return header;
}

float RecyclerDataSource::heightForHeader(RecyclerFrame* recycler, int section)
{
    if (section == 0)
        return 0;
    return 44;
}

RecyclerContentBox::RecyclerContentBox(RecyclerFrame* recycler)
    : Box(Axis::COLUMN)
    , recycler(recycler)
{
}

View* RecyclerContentBox::getNextFocus(FocusDirection direction, View* currentView)
{
    return this->recycler->getNextCellFocus(direction, currentView);
}

View* RecyclerFrame::getNextCellFocus(FocusDirection direction, View* currentView)
{
    void* parentUserData = currentView->getParentUserData();

    // Return nullptr immediately if focus direction mismatches the box axis (clang-format refuses to split it in multiple lines...)
    if ((this->contentBox->getAxis() == Axis::ROW && direction != FocusDirection::LEFT && direction != FocusDirection::RIGHT) || (this->contentBox->getAxis() == Axis::COLUMN && direction != FocusDirection::UP && direction != FocusDirection::DOWN))
    {
        View* next = getParentNavigationDecision(this, nullptr, direction);
        if (!next && hasParent())
            next = getParent()->getNextFocus(direction, this);
        return next;
    }

    // Traverse the children
    size_t offset = 1; // which way we are going in the children list

    if ((this->contentBox->getAxis() == Axis::ROW && direction == FocusDirection::LEFT) || (this->contentBox->getAxis() == Axis::COLUMN && direction == FocusDirection::UP))
    {
        offset = -1;
    }

    size_t currentFocusIndex = *((size_t*)parentUserData) + offset;
    View* currentFocus       = nullptr;

    while (!currentFocus && currentFocusIndex >= 0 && currentFocusIndex < this->cacheIndexPathData.size())
    {
        for (auto it : this->contentBox->getChildren())
        {
            if (*((size_t*)it->getParentUserData()) == currentFocusIndex)
            {
                currentFocus = it->getDefaultFocus();
                break;
            }
        }
        currentFocusIndex += offset;
    }

    currentFocus = getParentNavigationDecision(this, currentFocus, direction);
    if (!currentFocus && hasParent())
        currentFocus = getParent()->getNextFocus(direction, this);
    return currentFocus;
}

RecyclerFrame::RecyclerFrame()
{
    registerCell("brls::Header", []() { return RecyclerHeader::create(); });

    // Padding
    this->registerFloatXMLAttribute("paddingTop", [this](float value) {
        this->setPaddingTop(value);
    });

    this->registerFloatXMLAttribute("paddingRight", [this](float value) {
        this->setPaddingRight(value);
    });

    this->registerFloatXMLAttribute("paddingBottom", [this](float value) {
        this->setPaddingBottom(value);
    });

    this->registerFloatXMLAttribute("paddingLeft", [this](float value) {
        this->setPaddingLeft(value);
    });

    this->registerFloatXMLAttribute("padding", [this](float value) {
        this->setPadding(value);
    });

    this->setScrollingBehavior(ScrollingBehavior::CENTERED);

    // Create content box
    this->contentBox = new RecyclerContentBox(this);
    this->setContentView(this->contentBox);
}

RecyclerFrame::~RecyclerFrame()
{
    if (this->dataSource && this->deleteDataSource)
        delete dataSource;

    for (auto it : queueMap)
    {
        for (auto item : *it.second)
            delete item;
        delete it.second;
    }
}

void RecyclerFrame::setDataSource(RecyclerDataSource* source, bool deleteDataSource)
{
    if (this->dataSource && this->deleteDataSource)
        delete this->dataSource;

    this->dataSource = source;
    this->deleteDataSource = deleteDataSource;
    if (layouted)
        reloadData();
}

RecyclerDataSource* RecyclerFrame::getDataSource() const
{
    return this->dataSource;
}

void RecyclerFrame::reloadData()
{
    if (!layouted)
        return;

    // reloadData() mutates the child list, and every removeCell()/addCellAt()
    // invalidates the tree. View::invalidate() bubbles up to the parentless
    // root and runs YGNodeCalculateLayout() synchronously, whose NodeLayout
    // events call onLayout() back on this frame; while the frame size is
    // still settling (startup, sidebar fold) checkWidth() passes there and
    // reloadData() re-enters mid-mutation, corrupting the children/queue
    // state (cells inserted twice, queued cells still in the tree). Defer
    // nested requests and coalesce them into one rerun after this pass.
    if (mutationDepth > 0)
    {
        reloadPending = true;
        return;
    }

    // The rebuild below recycles every cell. If focus sits on one of them it
    // would be left dangling on a queued instance, whose highlight keeps
    // drawing at the stale detached position — the visible "shifted focus".
    // Remember the focused cell's flat index and restore it afterwards.
    View* focusedCell = nullptr;
    for (View* view = Application::getCurrentFocus(); view && view != this;
         view          = view->getParent())
    {
        if (view->getParent() == this->contentBox)
        {
            focusedCell = view;
            break;
        }
    }
    size_t focusedIndex =
        focusedCell ? *((size_t*)focusedCell->getParentUserData()) : 0;

    mutationDepth++;
    do
    {
        reloadPending = false;

        auto children = this->contentBox->getChildren();
        for (auto const& child : children)
        {
            queueReusableCell((RecyclerCell*)child);
            this->removeCell(child);
        }

        visibleMin = UINT_MAX;
        visibleMax = 0;

        renderedFrame            = Rect();
        renderedFrame.size.width = getWidth();

        setContentOffsetY(0, false);

        if (dataSource)
        {
            cacheCellFrames();
            Rect frame  = getLocalFrame();
            int counter = 0;
            for (int section = 0; section < dataSource->numberOfSections(this); section++)
            {
                for (int row = -1; row < dataSource->numberOfRows(this, section); row++)
                {
                    addCellAt(counter++, true);
                    if (renderedFrame.getMaxY() > frame.getMaxY())
                        break;
                }
            }

            selectRowAt(defaultCellFocus, false);
        }
    } while (reloadPending);
    mutationDepth--;

    if (focusedCell)
    {
        View* target = nullptr;
        for (View* child : contentBox->getChildren())
        {
            if (*((size_t*)child->getParentUserData()) == focusedIndex)
            {
                target = child;
                break;
            }
        }
        // The row is gone (list shrank): hand focus to the frame itself,
        // whose default focus lands on the selectRowAt() cell above.
        Application::giveFocus(target ? target : (View*)this);
    }
}

void RecyclerFrame::registerCell(std::string identifier, std::function<RecyclerCell*()> allocation)
{
    queueMap.insert(std::make_pair(identifier, new std::vector<RecyclerCell*>()));
    allocationMap.insert(std::make_pair(identifier, allocation));
}

RecyclerCell* RecyclerFrame::dequeueReusableCell(std::string identifier)
{
    RecyclerCell* cell = nullptr;
    auto it            = queueMap.find(identifier);

    if (it != queueMap.end())
    {
        std::vector<RecyclerCell*>* vector = it->second;
        if (!vector->empty())
        {
            cell = vector->back();
            vector->pop_back();
        }
        else
        {
            cell                  = allocationMap.at(identifier)();
            cell->reuseIdentifier = identifier;
            cell->detach();
        }
    }

    if (cell)
        cell->prepareForReuse();

    return cell;
}

// TODO: Implement it normally
void RecyclerFrame::selectRowAt(IndexPath indexPath, bool animated)
{
    size_t count    = 0;
    float offset = 0;

    // cacheFramesData holds one entry per cached cell (a header entry plus
    // one per row). A section with zero rows caches only its header, and a
    // focused row beyond the cached set must not read past the vector.
    for (size_t j = 0; j < indexPath.section; j++)
        for (int i = -1; i < (dataSource->numberOfRows(this, j)); i++)
        {
            if (count >= this->cacheFramesData.size())
                break;
            offset += this->cacheFramesData[count++].height;
        }

    for (int i = -1; i <= indexPath.row; i++)
    {
        if (count >= this->cacheFramesData.size())
            break;
        offset += this->cacheFramesData[count++].height;
    }

    offset -= this->getHeight() / 2;
    this->setContentOffsetY(offset, animated);
    this->cellsRecyclingLoop();

    for (View* view : contentBox->getChildren())
    {
        if (*((size_t*)view->getParentUserData()) == count - 1)
        {
            contentBox->setLastFocusedView(view);
            break;
        }
    }
}

void RecyclerFrame::queueReusableCell(RecyclerCell* cell)
{
    queueMap.at(cell->reuseIdentifier)->push_back(cell);
}

void RecyclerFrame::cacheCellFrames()
{
    cacheFramesData.clear();
    cacheIndexPathData.clear();
    Rect frame = getFrame();
    Point currentOrigin;

    if (dataSource)
    {
        for (int section = 0; section < dataSource->numberOfSections(this); section++)
        {
            for (int row = -1; row < dataSource->numberOfRows(this, section); row++)
            {
                cacheIndexPathData.push_back(IndexPath(section, row));

                float height = row == -1 ? dataSource->heightForHeader(this, section) : dataSource->heightForRow(this, IndexPath(section, row));

                if (height == -1)
                    height = estimatedRowHeight;

                cacheFramesData.push_back(Size(frame.getWidth(), height));
                currentOrigin.y += height;
            }
        }
        contentBox->setHeight(currentOrigin.y + paddingTop + paddingBottom);
    }
}

bool RecyclerFrame::checkWidth()
{
    // Per-instance: a function-static width was shared by every recycler, so a
    // newly shown list with the same width as the previous tab skipped its
    // first layout (focus highlight sat on the wrong pixels until resize).
    float width = getWidth();
    if ((int)oldWidth != (int)width && width != 0)
    {
        oldWidth = width;
        return true;
    }
    oldWidth = width;
    return false;
}

void RecyclerFrame::cellsRecyclingLoop()
{
    // Same reentrancy guard as reloadData(): removeCell()/addCellAt()
    // invalidate the recycler, which can synchronously re-enter onLayout()
    // and request reloadData() while the loops below iterate the child list.
    mutationDepth++;
    Rect visibleFrame = getVisibleFrame();

    while (true)
    {
        RecyclerCell* minCell = nullptr;
        for (auto it : contentBox->getChildren())
            if (*((size_t*)it->getParentUserData()) == visibleMin)
                minCell = (RecyclerCell*)it;

        if (!minCell || minCell->getDetachedPosition().y + minCell->getHeight() >= visibleFrame.getMinY())
            break;

        float cellHeight = minCell->getHeight();
        renderedFrame.origin.y += cellHeight;
        renderedFrame.size.height -= cellHeight;

        queueReusableCell(minCell);
        this->removeCell(minCell);

        Logger::debug("Cell #{} - destroyed", visibleMin);

        visibleMin++;
    }

    while (true)
    {
        RecyclerCell* maxCell = nullptr;
        for (auto it : contentBox->getChildren())
            if (*((size_t*)it->getParentUserData()) == visibleMax)
                maxCell = (RecyclerCell*)it;

        if (!maxCell || maxCell->getDetachedPosition().y <= visibleFrame.getMaxY())
            break;

        float cellHeight = maxCell->getHeight();
        renderedFrame.size.height -= cellHeight;

        queueReusableCell(maxCell);
        this->removeCell(maxCell);

        Logger::debug("Cell #{} - destroyed", visibleMax);

        visibleMax--;
    }

    while (visibleMin - 1 < cacheFramesData.size() && renderedFrame.getMinY() > visibleFrame.getMinY() - paddingTop)
    {
        int i = visibleMin - 1;
        addCellAt(i, false);
    }

    while (visibleMax + 1 < cacheFramesData.size() && renderedFrame.getMaxY() < visibleFrame.getMaxY() - paddingBottom)
    {
        int i = visibleMax + 1;
        addCellAt(i, true);
    }

    mutationDepth--;
    // Draw()-time entry point: a reload deferred above has no outer pass to
    // pick it up, so run it once we are back out of the mutation.
    if (reloadPending && mutationDepth == 0)
        reloadData();
}

void RecyclerFrame::addCellAt(size_t index, size_t downSide)
{
    IndexPath indexPath = cacheIndexPathData[index];

    RecyclerCell* cell;
    if (indexPath.row == -1)
        cell = dataSource->cellForHeader(this, indexPath.section);
    else
    {
        cell = dataSource->cellForRow(this, indexPath);
        cell->setLineBottom(1);
    }

    cell->setWidth(renderedFrame.getWidth() - paddingLeft - paddingRight);
    Point cellOrigin = Point(renderedFrame.getMinX() + paddingLeft,
        (downSide ? renderedFrame.getMaxY() : renderedFrame.getMinY() - cell->getHeight()) + paddingTop);

    cell->setDetachedPosition(cellOrigin.x, cellOrigin.y);
    cell->setIndexPath(indexPath);

    this->contentBox->getChildren().insert(this->contentBox->getChildren().end(), cell);

    // Allocate and set parent userdata
    size_t* userdata = (size_t*)malloc(sizeof(size_t));
    *userdata        = index;

    cell->setParent(this->contentBox, userdata);

    // Layout and events
    this->contentBox->invalidate();
    cell->View::willAppear();

    if (index < visibleMin)
        visibleMin = index;

    if (index > visibleMax)
        visibleMax = index;

    Rect cellFrame = cell->getFrame();

    if (!downSide)
        renderedFrame.origin.y -= cellFrame.getHeight();

    renderedFrame.size.height += cellFrame.getHeight();

    if (cellFrame.getHeight() != cacheFramesData[index].height)
    {
        float delta = cellFrame.getHeight() - cacheFramesData[index].height;
        contentBox->setHeight(contentBox->getHeight() + delta);
        cacheFramesData[index].height = cellFrame.getHeight();
    }

    Logger::debug("Cell #{} - added", index);
}

void RecyclerFrame::removeCell(View* view)
{
    if (!view)
        return;

    // Find the index of the view
    size_t index;
    bool found = false;
    auto& children = this->contentBox->getChildren();

    for (size_t i = 0; i < children.size(); i++)
    {
        View* child = children[i];

        if (child == view)
        {
            found = true;
            index = i;
            break;
        }
    }

    if (!found)
        return;

    // Remove it
    children.erase(children.begin() + index);

    view->willDisappear(true);

    this->invalidate();
}

void RecyclerFrame::onLayout()
{
    ScrollingFrame::onLayout();
    this->contentBox->setWidth(this->getWidth());
    if (checkWidth())
    {
        layouted = true;
        reloadData();
    }
}

void RecyclerFrame::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    // Sidebar collapse (and any other parent-width change) can leave recycled
    // cells at the previous setWidth: reloadData() from onLayout runs inside
    // Yoga's NodeLayout callback and the new width does not stick. Sync here,
    // after layout, so the focus ring matches the row.
    const float cellW = getWidth() - paddingLeft - paddingRight;
    if (cellW > 0)
    {
        for (View* child : contentBox->getChildren())
        {
            if ((int)child->getWidth() != (int)cellW)
                child->setWidth(cellW);
        }
    }
    cellsRecyclingLoop();
    ScrollingFrame::draw(vg, x, y, width, height, style, ctx);
}

void RecyclerFrame::setPadding(float padding)
{
    this->setPadding(padding, padding, padding, padding);
}

void RecyclerFrame::setPadding(float top, float right, float bottom, float left)
{
    paddingTop    = top;
    paddingRight  = right;
    paddingBottom = bottom;
    paddingLeft   = left;

    this->reloadData();
}

void RecyclerFrame::setPaddingTop(float top)
{
    paddingTop = top;
    this->reloadData();
}

void RecyclerFrame::setPaddingRight(float right)
{
    paddingRight = right;
    this->reloadData();
}

void RecyclerFrame::setPaddingBottom(float bottom)
{
    paddingBottom = bottom;
    this->reloadData();
}

void RecyclerFrame::setPaddingLeft(float left)
{
    paddingLeft = left;
    this->reloadData();
}

View* RecyclerFrame::create()
{
    return new RecyclerFrame();
}

} // namespace brls
