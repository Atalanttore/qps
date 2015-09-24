/**********************************************************************
** $Id: qttableview.cpp,v 1.115 2011/08/27 00:13:41 fasthyun Exp $
** Implementation of QtTableView class
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file contains a class moved out of the Qt GUI Toolkit API.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qttableview.h"

enum ScrollBarDirtyFlags
{
    verSteps = 0x02,
    verRange = 0x04,
    verValue = 0x08,
    horSteps = 0x20,
    horRange = 0x40,
    horValue = 0x80,
    verMask = 0x0F,
    horMask = 0xF0
};

QtTableView::QtTableView(QWidget *parent, const char *name)
    : QAbstractScrollArea(parent)
{
    nRows = nCols = 0; // zero rows/cols

    xCellOffs = yCellOffs = 0;   // zero offset
    xCellDelta = yCellDelta = 0; // zero cell offset
    xOffs = yOffs = 0;           // zero total pixel offset
    cellH = cellW = 0;           // user defined cell size
    vScrollBar = hScrollBar = 0; // no scroll bars
    tFlags = 0;
    sbDirty = 0;
    verSliding = false;
    verSnappingOff = false;
    horSliding = false;
    horSnappingOff = false;
    inSbUpdate = false;
    flag_view = 0;
    view = viewport();
    enablePaint = true;
    test = false;

    // setSizePolicy(QSizePolicy
    // (QSizePolicy::Expanding,QSizePolicy::Expanding));
    // setFocusPolicy(Qt::NoFocus);

    verticalScrollBar();   // created
    horizontalScrollBar(); // created
                           //	setFocusPolicy(Qt::NoFocus);
                           //	setFrameShape (QFrame::NoFrame);
                           //	setStyle(new QCleanlooksStyle); // compiz bug
                           //	setStyle(new QWindowsStyle); // repaint bug
}
/*
  Destroys the table view.
*/
QtTableView::~QtTableView()
{
    delete vScrollBar;
    delete hScrollBar;
}

/*
  Returns a pointer to the vertical scroll bar mainly so you can
  connect() to its signals.  Note that the scroll bar works in pixel
  values; use findRow() to translate to cell numbers.
*/

QScrollBar *QtTableView::verticalScrollBar() const
{
    QtTableView *that = (QtTableView *)this; // semantic const
    if (!vScrollBar)
    {
        QScrollBar *sb = QAbstractScrollArea::verticalScrollBar();
        sb->setFocusPolicy(Qt::NoFocus);
        connect(sb, SIGNAL(valueChanged(int)), SLOT(verSbValue(int)));
        connect(sb, SIGNAL(sliderMoved(int)), SLOT(verSbSliding(int)));
        connect(sb, SIGNAL(sliderReleased()), SLOT(verSbSlidingDone()));
        that->vScrollBar = sb;
        // vScrollBar = sb;
        return sb;
    }
    return vScrollBar;
}

QScrollBar *QtTableView::horizontalScrollBar() const
{
    QtTableView *that = (QtTableView *)this; // semantic const
    if (!hScrollBar)
    {
        QScrollBar *sb = QAbstractScrollArea::horizontalScrollBar();
        sb->setFocusPolicy(Qt::NoFocus);
        //	sb->setTracking( true ); // bar cause heads moving
        connect(sb, SIGNAL(valueChanged(int)), SLOT(horSbValue(int)));
        connect(sb, SIGNAL(sliderMoved(int)), SLOT(horSbSliding(int)));
        connect(sb, SIGNAL(sliderReleased()), SLOT(horSbSlidingDone()));
        that->hScrollBar = sb;
        return sb;
    }
    return hScrollBar;
}

/*
  Sets the number of rows of the table to \a rows (must be non-negative).
  Does not change topCell().
  The table repaints itself automatically if updatesEnabled() is set.

  numCols(), setNumCols(), numRows()
*/

void QtTableView::setNumRows(int rows)
{
    ///	view->setMaximumHeight(rows*cellH);
    if (rows < 0)
    {
        qWarning("QtTableView::setNumRows: (%s) Negative argument %d.",
                 "unnamed", rows);
        return;
    }
    if (nRows == rows)
        return;

    if (isVisible())
    {
        int oldLastVisible = lastRowVisible();
        int oldTopCell = topCell();
    }
    nRows = rows;
    updateScrollBars(verRange);
}

void QtTableView::setNumCols(int cols)
{
    if (cols < 0)
    {
        qWarning("QtTableView::setNumCols: (%s) Negative argument %d.",
                 "unnamed", cols);
        return;
    }
    if (nCols == cols)
        return;
    int oldCols = nCols;
    nCols = cols;
    updateScrollBars(horRange); // int maxCol = lastColVisible();
}

/*
  \fn int QtTableView::topCell() const
  Returns the index of the first row in the table that is visible in
  the view.  The index of the first row is 0.
  \sa leftCell(), setTopCell()
  Scrolls the table so that \a row becomes the top row.
  The index of the very first row is 0.
  \sa setYOffset(), setTopLeftCell(), setLeftCell()
*/

void QtTableView::setTopCell(int row)
{
    setTopLeftCell(row, -1);
    return;
}

/*
  \fn int QtTableView::leftCell() const
  Returns the index of the first column in the table that is visible in
  the view.  The index of the very leftmost column is 0.
  \sa topCell(), setLeftCell()
*/

/*
  Scrolls the table so that \a col becomes the leftmost
  column.  The index of the leftmost column is 0.
  \sa setXOffset(), setTopLeftCell(), setTopCell()
*/

void QtTableView::setLeftCell(int col)
{
    setTopLeftCell(-1, col);
    return;
}

/*
  Scrolls the table so that the cell at row \a row and colum \a
  col becomes the top-left cell in the view.  The cell at the extreme
  top left of the table is at position (0,0).
  \sa setLeftCell(), setTopCell(), setOffset()
*/

void QtTableView::setTopLeftCell(int row, int col)
{
    int newX = xOffs;
    int newY = yOffs;

    if (col >= 0)
    {
        if (cellW)
        {
            newX = col * cellW;
            if (newX > maxXOffset())
                newX = maxXOffset();
        }
        else
        {
            newX = 0;
            while (col)
                newX += cellWidth(--col); // optimize using current! ###
        }
    }
    if (row >= 0)
    {
        if (cellH)
        {
            newY = row * cellH;
            if (newY > maxYOffset())
                newY = maxYOffset();
        }
        else
        {
            newY = 0;
            while (row)
                newY += cellHeight(--row); // optimize using current! ###
        }
    }
    setOffset(newX, newY);
}

/*
  \fn int QtTableView::xOffset() const

  Returns the x coordinate in \e table coordinates of the pixel that is
  currently on the left edge of the view.

  \sa setXOffset(), yOffset(), leftCell() */

/*
  Scrolls the table so that \a x becomes the leftmost pixel in the view.
  The \a x parameter is in \e table coordinates.

  The interaction with \link setTableFlags() Tbl_snapToHGrid
  \endlink is tricky.

  \sa xOffset(), setYOffset(), setOffset(), setLeftCell()
*/

void QtTableView::setXOffset(int x) { setOffset(x, yOffset()); }

/*
  int QtTableView::yOffset() const

  Returns the y coordinate in \e table coordinates of the pixel that is
  currently on the top edge of the view.

  :setYOffset(), xOffset(), topCell()
*/

// SCROLL Code called by wheel
void QtTableView::setYOffset(int y) { setOffset(xOffset(), y, true); }

/*
  Scrolls the table so that \a (x,y) becomes the top-left pixel
  in the view. Parameters \a (x,y) are in \e table coordinates.

  The interaction with \link setTableFlags() Tbl_snapTo*Grid \endlink
  is tricky.  If \a updateScrBars is true, the scroll bars are
  updated.

  \sa xOffset(), yOffset(), setXOffset(), setYOffset(), setTopLeftCell()
*/

void QtTableView::setOffset(int x, int y, bool updateScrBars)
{
    if ((!testTableFlags(Tbl_snapToHGrid) || xCellDelta == 0) &&
        (!testTableFlags(Tbl_snapToVGrid) || yCellDelta == 0) &&
        (x == xOffs && y == yOffs))
        return;

    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    if (cellW)
    {
        if (x > maxXOffset())
            x = maxXOffset();
        xCellOffs = x / cellW;
        if (!testTableFlags(Tbl_snapToHGrid))
        {
            xCellDelta = (short)(x % cellW);
        }
        else
        {
            x = xCellOffs * cellW;
            xCellDelta = 0;
        }
    }
    else
    {
        int xn = 0, xcd = 0, col = 0;
        while (col < nCols - 1 && x >= xn + (xcd = cellWidth(col)))
        {
            xn += xcd;
            col++;
        }
        xCellOffs = col; //!!!
        if (testTableFlags(Tbl_snapToHGrid))
        {
            xCellDelta = 0;
            x = xn;
        }
        else
        {
            xCellDelta = (short)(x - xn);
        }
    }
    if (cellH)
    { // same cellHegiht
        if (y > maxYOffset())
            y = maxYOffset();
        yCellOffs = y / cellH; //********
        if (!testTableFlags(Tbl_snapToVGrid))
        {
            yCellDelta = (short)(y % cellH);
        }
        else
        {
            y = yCellOffs * cellH;
            yCellDelta = 0;
        }
    }
    else
    {
    }
    //	printf("y=%d yOffs=%d ", y,yOffs);
    int dx = (xOffs - x);
    int dy = (yOffs - y); //
    xOffs = x;
    yOffs = y;
    if (updatesEnabled() && isVisible())
    {
        // *** scroll() call update( desposal region ) !!
        // printf("scroll: xpixel=%d , ypixel=%d \n",xPixels,yPixels);
        scrollTrigger(dx, dy);
        view->scroll(dx, dy);
    }
    if (updateScrBars)
        updateScrollBars(verValue | horValue);
}

void QtTableView::scrollContentsBy(int dx, int dy)
{
    // view->update() //default
    // printf("scroll: dx=%d , dy=%d \n",dx,dy);
}
/*
  Moves the visible area of the table right by \a xPixels and
  down by \a yPixels pixels.  Both may be negative.
  \warning You might find that QScrollView offers a higher-level of
        functionality than using QtTableView and this function.
  setXOffset(), setYOffset(), setOffset(), setTopCell(), setLeftCell()
*/

/*
  \overload int QtTableView::cellWidth() const

  Returns the column width in pixels.	Returns 0 if the columns have
  variable widths.

  \sa setCellWidth(), cellHeight()
  Returns the width of column \a col in pixels.

  setCellWidth(), cellHeight(), totalWidth(), updateTableSize()
*/

// virtual
int QtTableView::cellWidth(int col) { return cellW; }

/*
  Sets the width in pixels of the table cells to \a cellWidth.

  Setting it to 0 means that the column width is variable.  When
  set to 0 (this is the default) QtTableView calls the virtual function
  cellWidth() to get the width.

  \sa cellWidth(), setCellHeight(), totalWidth(), numCols()
*/

void QtTableView::setCellWidth(int cellWidth)
{
    if (cellW == cellWidth)
        return;
#if defined(QT_CHECK_RANGE)
    if (cellWidth < 0 || cellWidth > SHRT_MAX)
    {
        qWarning("QtTableView::setCellWidth: (%s) Argument out of "
                 "range (%d)",
                 name("unnamed"), cellWidth);
        return;
    }
#endif
    cellW = (short)cellWidth;

    updateScrollBars(horSteps | horRange);
    if (updatesEnabled() && isVisible())
        update();
}

/*
  \overload int QtTableView::cellHeight() const

  Returns the row height, in pixels.  Returns 0 if the rows have
  variable heights.

  \sa setCellHeight(), cellWidth()

  Returns the height of row \a row in pixels.

  This function is virtual and must be reimplemented by subclasses that
  have variable cell heights.  Note that if the total table height
  changes, updateTableSize() must be called.

  \sa setCellHeight(), cellWidth(), totalHeight()
*/

int QtTableView::cellHeight(int) { return cellH; }

/*
  Sets the height in pixels of the table cells to \a cellHeight.

  Setting it to 0 means that the row height is variable.  When set
  to 0 (this is the default), QtTableView calls the virtual function
  cellHeight() to get the height.

  cellHeight(), setCellWidth(), totalHeight(), numRows()
*/
// call by htable:: fontChanged
void QtTableView::setCellHeight(int cellHeight)
{

    if (cellH == cellHeight)
        return;
#if defined(QT_CHECK_RANGE)
    if (cellHeight < 0 || cellHeight > SHRT_MAX)
    {
        qWarning("QtTableView::setCellHeight: (%s) Argument out of "
                 "range (%d)",
                 name("unnamed"), cellHeight);
        return;
    }
#endif
    cellH = (short)cellHeight;
    if (updatesEnabled() && isVisible())
        update();
    updateScrollBars(verSteps | verRange);
}

// using
int QtTableView::totalWidth()
{
    if (cellW)
    {
        return cellW * nCols;
    }
    else
    {
        int tw = 0;
        for (int i = 0; i < nCols; i++)
            tw += cellWidth(i);
        return tw;
    }
}

// using
int QtTableView::totalHeight()
{
    if (cellH)
    {
        return cellH * nRows;
    }
    else
    {
        int th = 0;
        for (int i = 0; i < nRows; i++)
            th += cellHeight(i);
        return th;
    }
}

/*
  \fn uint QtTableView::tableFlags() const

  Returns the union of the table flags that are currently set.

  \sa setTableFlags(), clearTableFlags(), testTableFlags()
*/

/*
  \fn bool QtTableView::testTableFlags( uint f ) const

  Returns true if any of the table flags in \a f are currently set,
  otherwise  false.

  \sa setTableFlags(), clearTableFlags(), tableFlags()
*/

/*
  Sets the table flags to \a f.

  If a flag setting changes the appearance of the table, the table is
  repainted if - and only if - updatesEnabled() is true.

  The table flags are mostly single bits, though there are some multibit
  flags for convenience. Here is a complete list:

  Tbl_vScrollBar - The table has a vertical scroll bar.
  Tbl_hScrollBar <dd> - The table has a horizontal scroll bar.
  Tbl_autoVScrollBar <dd> - The table has a vertical scroll bar if
  - and only if - the table is taller than the view.
  Tbl_autoHScrollBar <dd> The table has a horizontal scroll bar if
        - and only if - the table is wider than the view.
  Tbl_autoScrollBars <dd> - The union of the previous two flags.
  Tbl_clipCellPainting <dd> - The table uses QPainter::setClipRect() to
  make sure that paintCell() will not draw outside the cell
  boundaries.
  Tbl_cutCellsV <dd> - The table will never show part of a
  cell at the bottom of the table; if there is not space for all of
  a cell, the space is left blank.
  <dt> Tbl_cutCellsH <dd> - The table will never show part of a
  cell at the right side of the table; if there is not space for all of
  a cell, the space is left blank.
  <dt> Tbl_cutCells <dd> - The union of the previous two flags.
  <dt> Tbl_scrollLastHCell <dd> - When the user scrolls horizontally,
  let him/her scroll the last cell left until it is at the left
  edge of the view.  If this flag is not set, the user can only scroll
  to the point where the last cell is completely visible.
  <dt> Tbl_scrollLastVCell <dd> - When the user scrolls vertically, let
  him/her scroll the last cell up until it is at the top edge of
  the view.  If this flag is not set, the user can only scroll to the
  point where the last cell is completely visible.
  <dt> Tbl_scrollLastCell <dd> - The union of the previous two flags.
  <dt> Tbl_smoothHScrolling <dd> - The table scrolls as smoothly as
  possible when the user scrolls horizontally. When this flag is not
  set, scrolling is done one cell at a time.
  <dt> Tbl_smoothVScrolling <dd> - The table scrolls as smoothly as
  possible when scrolling vertically. When this flag is not set,
  scrolling is done one cell at a time.
  <dt> Tbl_smoothScrolling <dd> - The union of the previous two flags.
  <dt> Tbl_snapToHGrid <dd> - Except when the user is actually scrolling,
  the leftmost column shown snaps to the leftmost edge of the view.
  <dt> Tbl_snapToVGrid <dd> - Except when the user is actually
  scrolling, the top row snaps to the top edge of the view.
  <dt> Tbl_snapToGrid <dd> - The union of the previous two flags.
  </dl>

  You can specify more than one flag at a time using bitwise OR.

  Example:
    setTableFlags( Tbl_smoothScrolling | Tbl_autoScrollBars );

  \warning The cutCells options (\c Tbl_cutCells, \c Tbl_cutCellsH and
  Tbl_cutCellsV) may cause painting problems when scrollbars are
  enabled. Do not combine cutCells and scrollbars.
  clearTableFlags(), testTableFlags(), tableFlags()
*/

void QtTableView::setTableFlags(uint f)
{
    f = (f ^ tFlags) & f; // clear flags already set
    tFlags |= f;

    bool updateOn = updatesEnabled();
    setAutoUpdate(false);

    uint repaintMask = Tbl_cutCellsV | Tbl_cutCellsH;

    if (f & Tbl_autoVScrollBar)
    {
        updateScrollBars(verRange);
    }
    if (f & Tbl_autoHScrollBar)
    {
        updateScrollBars(horRange);
    }
    if (f & Tbl_scrollLastHCell)
    {
        updateScrollBars(horRange);
    }
    if (f & Tbl_scrollLastVCell)
    {
        updateScrollBars(verRange);
    }
    if (f & Tbl_snapToHGrid)
    {
        updateScrollBars(horRange);
    }
    if (f & Tbl_snapToVGrid)
    {
        updateScrollBars(verRange);
    }
    if (f & Tbl_snapToGrid)
    { // Note: checks for 2 flags
        if (((f & Tbl_snapToHGrid) != 0 &&
             xCellDelta != 0) || // have to scroll?
            ((f & Tbl_snapToVGrid) != 0 && yCellDelta != 0))
        {
            repaintMask |= Tbl_snapToGrid; // repaint table
        }
    }

    if (updateOn)
    {
        setAutoUpdate(true);
        updateScrollBars();
        if (isVisible() && (f & repaintMask))
            update();
    }
}

/*
  Clears the \link setTableFlags() table flags\endlink that are set
  in \a f.

  Example (clears a single flag):
  \code
    clearTableFlags( Tbl_snapToGrid );
  \endcode

  The default argument clears all flags.

  \sa setTableFlags(), testTableFlags(), tableFlags()
*/

void QtTableView::clearTableFlags(uint f)
{
    f = (f ^ ~tFlags) & f; // clear flags that are already 0
    tFlags &= ~f;

    bool updateOn = updatesEnabled();
    setAutoUpdate(false);

    uint repaintMask = Tbl_cutCellsV | Tbl_cutCellsH;

    if (f & Tbl_scrollLastHCell)
    {
        int maxX = maxXOffset();
        if (xOffs > maxX)
        {
            setOffset(maxX, yOffs);
            repaintMask |= Tbl_scrollLastHCell;
        }
        updateScrollBars(horRange);
    }
    if (f & Tbl_scrollLastVCell)
    {
        int maxY = maxYOffset();
        if (yOffs > maxY)
        {
            setOffset(xOffs, maxY);
            repaintMask |= Tbl_scrollLastVCell;
        }
        updateScrollBars(verRange);
    }
    if (f & Tbl_smoothScrolling)
    { // Note: checks for 2 flags
        if (((f & Tbl_smoothHScrolling) != 0 &&
             xCellDelta != 0) || // must scroll?
            ((f & Tbl_smoothVScrolling) != 0 && yCellDelta != 0))
        {
            repaintMask |= Tbl_smoothScrolling; // repaint table
        }
    }
    if (f & Tbl_snapToHGrid)
    {
        updateScrollBars(horRange);
    }
    if (f & Tbl_snapToVGrid)
    {
        updateScrollBars(verRange);
    }
    if (updateOn)
    {
        setAutoUpdate(true);
        updateScrollBars(); // returns immediately if nothing to do
        if (isVisible() && (f & repaintMask))
            update(); // repaint();
    }
}

/*
  Sets the auto-update option of the table view to  enable.

  If enable is true (this is the default), the view updates itself
  automatically whenever it has changed in some way (for example, when a
  setTableFlags() flag\endlink is changed).
*/

void QtTableView::setAutoUpdate(bool enable)
{
    enablePaint = enable;
    // updatesEnabled= enable ;

    // setAttribute(Qt::WA_ForceUpdatesDisabled, !enable);
    ////    setAttribute(Qt::WA_UpdatesDisabled, !enable);
    //     d->setUpdatesEnabled_helper(enable);

    return;

    if (updatesEnabled() == enable)
        return;
    setUpdatesEnabled(enable); // update() call
    if (enable)
    {
        //  updateScrollBars();
    }
}

/*
  Returns the index of the last (bottom) row in the view.
  The index of the first row is 0.

  If no rows are visible it returns -1.	 This can happen if the
  view is too small for the first row and Tbl_cutCellsV is set.

  \sa lastColVisible()
 */

int QtTableView::lastRowVisible() const
{
    int cellMaxY;
    int row = findRawRow(maxViewY(), &cellMaxY);
    if (row == -1 || row >= nRows)
    {                    // maxViewY() past end?
        row = nRows - 1; // yes: return last row
    }
    else
    {
        if (testTableFlags(Tbl_cutCellsV) && cellMaxY > maxViewY())
        {
            if (row == yCellOffs) // cut by right margin?
                return -1;        // yes, nothing in the view
            else
                row = row - 1; // cut by margin, one back
        }
    }
    return row;
}

/*
  Returns the index of the last (right) column in the view.
  The index of the first column is 0.

  If no columns are visible it returns -1.  This can happen if the
  view is too narrow for the first column and Tbl_cutCellsH is set.

  \sa lastRowVisible()
*/

/*
int QtTableView::lastVisibleCol() const
{
        int cellMaxX;
        int col = findRawCol( maxViewX(), &cellMaxX );
        if ( col == -1 || col >= nCols ) {		// maxViewX() past end?
                col = nCols - 1;			// yes: return last col
        } else {
                col = col - 1;			// cell by margin, one back
        }
        return col;
} */

int QtTableView::lastColVisible() const
{
    int cellMaxX;
    int col = findRawCol(maxViewX(), &cellMaxX);
    if (col == -1 || col >= nCols)
    {                    // maxViewX() past end?
        col = nCols - 1; // yes: return last col
    }
    else
    {
        if (testTableFlags(Tbl_cutCellsH) && cellMaxX > maxViewX())
        {
            if (col == xCellOffs) // cut by bottom margin?
                return -1;        // yes, nothing in the view
            else
                col = col - 1; // cell by margin, one back
        }
    }
    return col;
}

/*
  Returns true if \a row is at least partially visible.
  \sa colIsVisible()
*/

bool QtTableView::rowIsVisible(int row) const { return rowYPos(row, 0); }

/*
  Returns true if \a col is at least partially visible.
  \sa rowIsVisible()
*/

bool QtTableView::colIsVisible(int col) const { return colXPos(col, 0); }

/*
  \internal
  This internal slot is connected to the horizontal scroll bar's
  QScrollBar::valueChanged() signal.

  Moves the table horizontally to offset \a val without updating the
  scroll bar.
*/

void QtTableView::horSbValue(int val)
{
    if (horSliding)
    {
        horSliding = false;
        if (horSnappingOff)
        {
            horSnappingOff = false;
            tFlags |= Tbl_snapToHGrid;
        }
    }
    setOffset(val, yOffs, false);
}

/*
  \internal
  This internal slot is connected to the horizontal scroll bar's
  QScrollBar::sliderMoved() signal.

  Scrolls the table smoothly horizontally even if \c Tbl_snapToHGrid is set.
*/

void QtTableView::horSbSliding(int val)
{
    if (testTableFlags(Tbl_snapToHGrid) && testTableFlags(Tbl_smoothHScrolling))
    {
        tFlags &= ~Tbl_snapToHGrid; // turn off snapping while sliding
        setOffset(val, yOffs, false);
        tFlags |= Tbl_snapToHGrid; // turn on snapping again
    }
    else
    {
        setOffset(val, yOffs, false);
    }
}

/*
  \internal
  This internal slot is connected to the horizontal scroll bar's
  QScrollBar::sliderReleased() signal.
*/

void QtTableView::horSbSlidingDone()
{
    if (testTableFlags(Tbl_snapToHGrid) && testTableFlags(Tbl_smoothHScrolling))
        ; //	snapToGrid( true,  false );
}

/*
  \internal
  This internal slot is connected to the vertical scroll bar's
  QScrollBar::valueChanged() signal.

  Moves the table vertically to offset \a val without updating the
  scroll bar.
 */

void QtTableView::verSbValue(int val)
{
    if (verSliding)
    {
        verSliding = false;
        if (verSnappingOff)
        {
            verSnappingOff = false;
            tFlags |= Tbl_snapToVGrid;
        }
    }
    setOffset(xOffs, val, false);
}

/*
  \internal
  This internal slot is connected to the vertical scroll bar's
  QScrollBar::sliderMoved() signal.

  Scrolls the table smoothly vertically even if \c Tbl_snapToVGrid is set.
*/

void QtTableView::verSbSliding(int val)
{
    if (testTableFlags(Tbl_snapToVGrid) && testTableFlags(Tbl_smoothVScrolling))
    {
        tFlags &= ~Tbl_snapToVGrid; // turn off snapping while sliding
        setOffset(xOffs, val, false);
        tFlags |= Tbl_snapToVGrid; // turn on snapping again
    }
    else
    {
        setOffset(xOffs, val, false);
    }
}

/*
  \internal
  This internal slot is connected to the vertical scroll bar's
  QScrollBar::sliderReleased() signal.
*/

void QtTableView::verSbSlidingDone()
{
    if (testTableFlags(Tbl_snapToVGrid) && testTableFlags(Tbl_smoothVScrolling))
        ; // snapToGrid(  false, true );
}

/*
  This virtual function is called before painting of table cells
  is started. It can be reimplemented by subclasses that want to
  to set up the painter in a special way and that do not want to
  do so for each cell.
*/

/*
  Handles paint events, for the table view.
  Calls paintCell() for the cells that needs to be repainted.
*/
// work?
void QtTableView::repaintCell(int row, int col, bool usecache) // false
{
    //
    static int c = 0;
    int xPos, yPos;
    if (!colXPos(col, &xPos))
        return;
    if (!rowYPos(row, &yPos))
        return;

    QRect uR = QRect(xPos, yPos, cellWidth(col), cellHeight(row));
    // printf("repaintCell() %d,
    // [%d,%d,%d,%d]\n",c++,uR.x(),uR.y(),uR.width(),uR.height() );
    view->repaint(uR.intersected(viewRect())); // slow
    // view->update( uR.intersect(viewRect()));
}

// using
void QtTableView::repaintRow(int row)
{
    int y;
    if (rowYPos(row, &y))
    {
        // view->repaint(minViewX(),y,viewWidth(),cellHeight());
        view->update(minViewX(), y, viewWidth(), cellHeight());
    };
}

extern QThread *thread_main;
void QtTableView::paintEvent(QPaintEvent *e)
{
    static int count = 0;

    ////if(thread_main!=thread())
    ///	printf("Error : main_thread(%X) != paint_thread(%X) report this
    /// message!!!\n",thread_main,thread());

    /// if ( !isVisible() or !enablePaint )	return;
    checkProfile(); // check cache, current_get

    if (!isVisible())
        return;

    QRect viewR = viewRect();
    bool flag_fullpainting = false;
    QRect updateR = e->rect(); // update rectangle

    QPainter paint(viewport());
    //	printf("Qps: %s::paintEvent()
    // count=%d\n",objectName().toAscii().data(),count++);
    //	printf("%s
    //[%d,%d,%d,%d];\n",objectName().toAscii().data(),updateR.x(),updateR.y(),updateR.width(),updateR.height());

    /*  possible?
            int maxVX = maxXOffset(), maxVY = maxYOffset();
            if ( updateR.right() > maxVX )	updateR.setRight( maxVX );
            if ( updateR.bottom() > maxVY ) updateR.setBottom( maxVY );
    */

    if (updateR.width() < viewR.width() or updateR.height() < viewR.height())
    {
        // when: Scroll_up,down and Selection
        // printf("partial: %d!=%d,
        // %d!=%d\n",updateR.width(),viewR.width(),updateR.height(),viewR.height());
        // printf("viewR (%d,%d,%d,%d)
        // \n",viewR.x(),viewR.y(),viewR.width(),viewR.height());
    }
    else
    {
        flag_fullpainting = true;
        ///	printf("%s::paintEvent() fullpainting
        ///%d\n",objectName().toAscii().data(),count++);
    }

    int firstRow = findRow(updateR.y());
    int firstCol = findCol(updateR.x());

    int xStart, yStart;

    if (!colXPos(firstCol, &xStart))
    {
        // right empty area of table
        // paint.eraseRect( updateR ); // erase area outside cells but
        // in view
        // printf("colXPos null\n");
        eraseRight(&paint, updateR);
        return;
    }

    // if ( !rowYPos( firstRow, &yStart ) || !colXPos( firstCol, &xStart ) )
    if (!rowYPos(firstRow, &yStart))
    { // get firstRow
        // printf("eraseRect()\n");
        paint.eraseRect(updateR);
        return;
    }

    int maxX = updateR.right();  // x2
    int maxY = updateR.bottom(); // y2
    int row = firstRow;
    int col;
    int yPos = yStart;
    int xPos = maxX + 1; // in case the while() is empty
    int nextX;
    int nextY;

    // printf("frow=%d,fcol=%d\n",firstRow,firstCol);
    // void 	(QtTableView::*painT)( QPainter *, int row, int col
    // ,bool
    // update);
    // painT=&QtTableView::paintCell;
    // paint.setClipRect( 0,0,viewR.width()-30,viewR.height() ); //enable,
    // font
    // not clip

    paint.setClipRect(viewR); // enable, font not clip (less Qt-4.3.x)
    // paint.setClipRect(updateR); //enable, font not clip

    while (yPos <= maxY and row < nRows)
    { // row=...5,6,7....
        nextY = yPos + cellHeight();
        col = firstCol;
        xPos = xStart;
        while (xPos < maxX and col < nCols)
        {
            QRect cell;
            int width = cellWidth(col);
            nextX = xPos + width;
            cell.setRect(xPos, yPos, width, cellH);
            tmp_size = viewR.intersected(cell).size();
            tmp_x = xPos;
            {
                paint.translate(xPos, yPos); // (0,0) 	// for subclass
                //(*this.*painT)( &paint, row, col ,
                // flag_use_cache);
                paintCell(&paint, row, col, false);
                paint.translate(-xPos, -yPos); // paint.translate(0,0);
            }
            col++;
            xPos = nextX;
        }
        row++;
        yPos = nextY;
    }

    //	printf("%s: xoff=%d ,yoff=%d
    //\n",objectName().toAscii().data(),xOffs,
    // yOffs);

    // while painting we have to erase any areas in the view that
    // are not covered by cells but are covered by the paint event
    // rectangle these must be erased. We know that xPos is the last
    // x pixel updated + 1 and that yPos is the last y pixel updated + 1.
    if (xPos <= maxX)
    {
        QRect r = viewR;
        r.setLeft(xPos);
        r.setBottom(yPos < maxY ? yPos : maxY);

        // QRect ir=r.intersect( updateR );
        eraseRight(&paint, r); //????????
    }

    if (yPos <= maxY)
    {
        QRect r = viewR;
        r.setTop(yPos);
        paint.eraseRect(r.intersected(updateR));
    }
}

void QtTableView::repaintChanged() // only fullpainting
{

    if (0)
    {
        //	printf("repaintChanged()\n");
        test = true;
        //	viewport()->setAutoFillBackground (false);
        viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
        viewport()->repaint();
        viewport()->setAttribute(Qt::WA_OpaquePaintEvent, false);
        test = false;
    }

    if (!isVisible())
        return;
    bool flag_fullpainting = false;

    QRect updateR = viewRect();
    QRect viewR = viewRect();

    flag_fullpainting = true;

    int firstRow = findRow(updateR.y());
    int firstCol = findCol(updateR.x());

    int xStart, yStart;

    if (!colXPos(firstCol, &xStart))
    {
        // right empty area of table
        printf("b\n");
        view->update(updateR);
        return;
    }

    // if ( !rowYPos( firstRow, &yStart ) || !colXPos( firstCol, &xStart ) )
    if (!rowYPos(firstRow, &yStart))
    { // get firstRow
        /// printf("a\n");
        view->update(updateR);
        return;
    }

    int maxX = updateR.right();  // x2
    int maxY = updateR.bottom(); // y2
    int row = firstRow;
    int col;
    int yPos = yStart;
    int xPos = maxX + 1; // in case the while() is empty
    int nextX;
    int nextY;

    while (yPos <= maxY and row < nRows)
    { // row=...5,6,7....
        nextY = yPos + cellHeight();
        col = firstCol;
        xPos = xStart;
        while (xPos < maxX and col < nCols)
        {
            QRect cell;
            int width = cellWidth(col);
            nextX = xPos + width;
            cell.setRect(xPos, yPos, width, cellH);
            int ctl = 0;
            tmp_size = viewR.intersected(cell).size();

            if (isCellChanged(row, col))
            {
                if (col == firstCol)
                {
                    repaintRow(row); //  speed up!  update()...
                    //	view->update(minViewX(),y,viewWidth(),cellHeight());
                    ///	printf("row %d\n",row);
                    break;
                }

                // printf("row %d col=%d\n",row,col);
                repaintCell(row, col, false);
            }
            col++;
            xPos = nextX;
        }
        row++;
        yPos = nextY;
    }

    if (xPos <= maxX)
    {
        QRect r = viewR;
        r.setLeft(xPos);
        r.setBottom(yPos < maxY ? yPos : maxY);
        view->repaint(r.intersected(updateR));
    }
    if (yPos <= maxY)
    {
        QRect r = viewR;
        r.setTop(yPos);
        view->repaint(r.intersected(updateR)); // why? CPU +2~3% -> rect.unite()
    }

    return;

    int rows = numRows();
    int cols = numCols();
    int left = leftCell(), right = lastColVisible();
    int top = topCell(), bottom = lastRowVisible();

    if (right >= cols)
        right = cols - 1; //???
    if (bottom >= rows)
        bottom = rows - 1;
    // if width[col] be changed ,then the right of [col] should be repainted
    // !
    // repaintColumns(c,-1);
    // printf("left=%d \n",left);

    for (int r = top; r <= bottom; r++)
    {
        for (int c = left; c <= right; c++)
        {
            // if(isCellChanged(r,c)) 	repaintCell(r, c,false);
        }
    }
}

void QtTableView::resizeEvent(QResizeEvent *e)
{
    //    printf("QtTableView::resize [%d,%d] \n",width(),height());
    // QAbstractScrollArea::resizeEvent(e);
    updateScrollBars(horValue | verValue | horSteps | horRange | verSteps |
                     verRange);
    return;
}

// BOTTLENECK?
int QtTableView::findRawRow(int yPos, int *cellMaxY, int *cellMinY,
                            bool goOutsideView) const
{
    int r = -1;
    if (nRows == 0)
        return r;
    if (goOutsideView || (yPos >= minViewY() && yPos <= maxViewY()))
    {
        if (yPos < minViewY())
        {
#if defined(QT_CHECK_RANGE)
            qWarning("QtTableView::findRawRow: (%s) internal error: "
                     "yPos < minViewY() && goOutsideView "
                     "not supported. (%d,%d)",
                     name("unnamed"), yPos, yOffs);
#endif
            return -1;
        }
        if (cellH)
        {                                                 // uniform cell height
            r = (yPos - minViewY() + yCellDelta) / cellH; // cell offs from top
            if (cellMaxY)
                *cellMaxY = (r + 1) * cellH + minViewY() - yCellDelta - 1;
            if (cellMinY)
                *cellMinY = r *cellH + minViewY() - yCellDelta;
            r += yCellOffs; // absolute cell index
        }
        else
        { // variable cell height
            QtTableView *tw = (QtTableView *)this;
            r = yCellOffs;
            int h = minViewY() - yCellDelta; //##arnt3
            int oldH = h;
            Q_ASSERT(r < nRows);
            while (r < nRows)
            {
                oldH = h;
                h += tw->cellHeight(r); // Start of next cell
                if (yPos < h)
                    break;
                r++;
            }
            if (cellMaxY)
                *cellMaxY = h - 1;
            if (cellMinY)
                *cellMinY = oldH;
        }
    }
    return r;
}

// return vitrual col.
int QtTableView::findRawCol(int xPos, int *cellMaxX, int *cellMinX,
                            bool goOutsideView) const
{
    int c = -1;
    if (nCols == 0)
        return c;
    if (goOutsideView || (xPos >= minViewX() && xPos <= maxViewX()))
    {
        if (xPos < minViewX())
        {
#if defined(QT_CHECK_RANGE)
            qWarning("QtTableView::findRawCol: (%s) internal error: "
                     "xPos < minViewX() && goOutsideView "
                     "not supported. (%d,%d)",
                     name("unnamed"), xPos, xOffs);
#endif
            return -1;
        }
        if (cellW)
        {                                                 // uniform cell width
            c = (xPos - minViewX() + xCellDelta) / cellW; // cell offs from left
            if (cellMaxX)
                *cellMaxX = (c + 1) * cellW + minViewX() - xCellDelta - 1;
            if (cellMinX)
                *cellMinX = c *cellW + minViewX() - xCellDelta;
            c += xCellOffs; // absolute cell index
        }
        else
        { // variable cell width
            QtTableView *tw = (QtTableView *)this;
            c = xCellOffs;
            int w = minViewX() - xCellDelta; //##arnt3
            int oldW = w;
            Q_ASSERT(c < nCols);
            while (c < nCols)
            {
                oldW = w;
                w += tw->cellWidth(c); // Start of next cell
                if (xPos < w)
                    break;
                c++;
            }
            if (cellMaxX)
                *cellMaxX = w - 1;
            if (cellMinX)
                *cellMinX = oldW;
        }
    }
    return c;
}

/*
  Returns the index of the row at position \a yPos, where \a yPos is in
  \e widget coordinates.  Returns -1 if \a yPos is outside the valid
  range.

  \sa findCol(), rowYPos()
*/

int QtTableView::findRow(int yPos) const
{
    int cellMaxY;
    int row = findRawRow(yPos, &cellMaxY);
    if (testTableFlags(Tbl_cutCellsV) && cellMaxY > maxViewY())
        row = -1; //  cell cut by bottom margin
    if (row >= nRows)
        row = -1;
    return row;
}

/*
  Returns the index of the column at position \a xPos, where \a xPos is
  in \e widget coordinates.  Returns -1 if \a xPos is outside the valid
  range.

  \sa findRow(), colXPos()
*/

int QtTableView::findCol(int xPos) const
{
    int cellMaxX;
    int col = findRawCol(xPos, &cellMaxX);
    if (testTableFlags(Tbl_cutCellsH) && cellMaxX > maxViewX())
        col = -1; //  cell cut by right margin
    if (col >= nCols)
        col = -1;
    return col;
}

// testing
int QtTableView::findColNoMinus(int xPos) const
{
    int col = findCol(xPos);

    if (col < 0)
    {
        if (xPos < 0)
            col = 0;
        else
            col = nCols - 1;
    }
    return col;
}
/*
  Computes the position in the widget of row \a row.

  Returns true and stores the result in \a *yPos (in \e widget
  coordinates) if the row is visible.  Returns  false and does not modify
  \a *yPos if \a row is invisible or invalid.

  \sa colXPos(), findRow()
*/

// return :  false means out of bound
bool QtTableView::rowYPos(int row, int *yPos) const
{
    int y;
    if (row >= yCellOffs)
    {
        if (cellH)
        {
            int lastVisible = lastRowVisible();
            if (row > lastVisible || lastVisible == -1)
                return false;
            y = (row - yCellOffs) * cellH + minViewY() - yCellDelta;
        }
        else
        {
            //##arnt3
            y = minViewY() - yCellDelta; // y of leftmost cell in view
            int r = yCellOffs;
            QtTableView *tw = (QtTableView *)this;
            int maxY = maxViewY();
            while (r < row && y <= maxY)
                y += tw->cellHeight(r++);
            if (y > maxY)
                return false;
        }
    }
    else
    {
        return false;
    }
    if (yPos)
        *yPos = y;
    return true;
}

/*
  Computes the position in the widget of column col.

  Returns true and stores the result in \a *xPos (in \e widget
  coordinates) if the column is visible.  Returns  false and does not
  modify \a *xPos if \a col is invisible or invalid.

  \sa rowYPos(), findCol()
*/

bool QtTableView::colXPos(int col, int *xPos) const
{
    int x;
    if (col >= xCellOffs)
    {
        if (cellW)
        {
            int lastVisible = lastColVisible();
            if (col > lastVisible || lastVisible == -1)
                return false;
            x = (col - xCellOffs) * cellW + minViewX() - xCellDelta;
        }
        else
        {
            //##arnt3
            x = minViewX() - xCellDelta; // x of uppermost cell in view
            int c = xCellOffs;
            QtTableView *tw = (QtTableView *)this;
            int maxX = maxViewX();
            while (c < col && x <= maxX)
                x += tw->cellWidth(c++);
            if (x > maxX)
                return false;
        }
    }
    else
    {
        return false;
    }
    if (xPos)
        *xPos = x;
    return true;
}

QRect QtTableView::viewRect() const
{
    return viewport()->rect();
    // return QRect( 0, 0, viewWidth(), viewHeight() );
}

int QtTableView::minViewX() const
{
    return 0; // frameWidth();
}

int QtTableView::minViewY() const
{
    return 0; // view->frameWidth();
}

/*
  Returns the rightmost pixel of the table view in \e view
  coordinates.	This excludes the frame and any scroll bar, but
  includes blank pixels to the right of the visible table data.

  \sa maxViewY(), viewWidth(),
*/

int QtTableView::maxViewX() const { return viewport()->width(); }

/*
  Returns the bottom pixel of the table view in \e view
  coordinates.	This excludes the frame and any scroll bar, but
  includes blank pixels below the visible table data.

  \sa maxViewX(), viewHeight(),
*/

int QtTableView::maxViewY() const { return viewport()->height(); }

/*
  Returns the width of the table view, as such, in \e view
  coordinates.  This does not include any header, scroll bar or frame,
  but it does include background pixels to the right of the table data.

  minViewX() maxViewX(), viewHeight(),viewRect()
*/

int QtTableView::viewWidth() const { return maxViewX(); }

int QtTableView::viewHeight() const
{
    return maxViewY(); /// - minViewY() + 1;
}

/*
  \fn void QtTableView::updateScrollBars()
  Updates the scroll bars' contents and presence to match the table's
  state.  Generally, you should not need to call this.
  \sa setTableFlags()
  Updates the scroll bars' contents and presence to match the table's
  state \c or \a f.
  \sa setTableFlags()
*/

void QtTableView::updateScrollBars(uint f)
{
    sbDirty = sbDirty | f;
    // if ( inSbUpdate )	return;
    // inSbUpdate = true;

    if (yOffset() > 0 && testTableFlags(Tbl_autoVScrollBar) &&
        !testTableFlags(Tbl_vScrollBar))
    {
        setYOffset(0); //????????
    }
    if (xOffset() > 0 && testTableFlags(Tbl_autoHScrollBar) &&
        !testTableFlags(Tbl_hScrollBar))
    {
        setXOffset(0); //?????
    }
    if (!isVisible())
    {
        inSbUpdate = false;
        return;
    }

    if (testTableFlags(Tbl_hScrollBar) && (sbDirty & horMask) != 0)
    {
        if (sbDirty & horSteps)
        {
            if (cellW)
                hScrollBar->setSingleStep(qMin((int)cellW, viewWidth() / 2));
            else
                hScrollBar->setSingleStep(16);
            hScrollBar->setPageStep(viewWidth());
        }

        if (sbDirty & horRange)
        {
            hScrollBar->setRange(0, maxXOffset());
        }
        if (sbDirty & horValue)
            hScrollBar->setValue(xOffs);
    }

    if (testTableFlags(Tbl_vScrollBar) && (sbDirty & verMask) != 0)
    {
        if (sbDirty & verSteps)
        {
            if (cellH)
                vScrollBar->setSingleStep(qMin((int)cellH, viewHeight() / 2));
            else
                vScrollBar->setSingleStep(16); // fttb! ###
            vScrollBar->setPageStep(viewHeight());
        }

        if (sbDirty & verRange)
            vScrollBar->setRange(0, maxYOffset());

        if (sbDirty & verValue)
            vScrollBar->setValue(yOffs);
    }

    sbDirty = 0;
    inSbUpdate = false;
}

/*
  Updates the scroll bars and internal state.

  Call this function when the table view's total size is changed;
  typically because the result of cellHeight() or cellWidth() have changed.
  This function does not repaint the widget.
*/
//->  updateViewSize();
void QtTableView::updateTableSize()
{
    updateScrollBars(horSteps | horRange | verSteps | verRange);
    return;
    bool updateOn = updatesEnabled();
    setAutoUpdate(false);
    int xofs = xOffset();
    xOffs++; // so that setOffset will not return immediately
    setOffset(xofs, yOffset(), false); // to calculate internal state correctly
    setAutoUpdate(updateOn);
}

/*
  Returns the maximum horizontal offset within the table of the
  view's left edge in \e table coordinates.

  This is used mainly to set the horizontal scroll bar's range.

  \sa maxColOffset(), maxYOffset(), totalWidth()
*/

int QtTableView::maxXOffset()
{
    int tw = totalWidth();
    int maxOffs;
    if (testTableFlags(Tbl_scrollLastHCell))
    {
        if (nCols != 1)
            maxOffs = tw - (cellW ? cellW : cellWidth(nCols - 1));
        else
            maxOffs = tw - viewWidth();
    }
    else
    {
        if (testTableFlags(Tbl_snapToHGrid))
        {
            if (cellW)
            {
                maxOffs = tw - (viewWidth() / cellW) * cellW;
            }
            else
            {
                int goal = tw - viewWidth();
                int pos = tw;
                int nextCol = nCols - 1;
                int nextCellWidth = cellWidth(nextCol);
                while (nextCol > 0 && pos > goal + nextCellWidth)
                {
                    pos -= nextCellWidth;
                    nextCellWidth = cellWidth(--nextCol);
                }
                if (goal + nextCellWidth == pos)
                    maxOffs = goal;
                else if (goal < pos)
                    maxOffs = pos;
                else
                    maxOffs = 0;
            }
        }
        else
        {
            maxOffs = tw - viewWidth();
        }
    }
    return maxOffs > 0 ? maxOffs : 0;
}

/*
  Returns the maximum vertical offset within the table of the
  view's top edge in \e table coordinates.

  This is used mainly to set the vertical scroll bar's range.
  \sa maxRowOffset(), maxXOffset(), totalHeight()
*/

int QtTableView::maxYOffset()
{
    int th = totalHeight();
    int maxOffs;
    if (testTableFlags(Tbl_scrollLastVCell))
    {
        if (nRows != 1)
            maxOffs = th - (cellH ? cellH : cellHeight(nRows - 1));
        else
            maxOffs = th - viewHeight();
    }
    else
    {
        if (testTableFlags(Tbl_snapToVGrid))
        {
            if (cellH)
            {
                maxOffs = th - (viewHeight() / cellH) * cellH;
            }
            else
            {
                int goal = th - viewHeight();
                int pos = th;
                int nextRow = nRows - 1;
                int nextCellHeight = cellHeight(nextRow);
                while (nextRow > 0 && pos > goal + nextCellHeight)
                {
                    pos -= nextCellHeight;
                    nextCellHeight = cellHeight(--nextRow);
                }
                if (goal + nextCellHeight == pos)
                    maxOffs = goal;
                else if (goal < pos)
                    maxOffs = pos;
                else
                    maxOffs = 0;
            }
        }
        else
        {
            maxOffs = th - viewHeight();
        }
    }
    return maxOffs > 0 ? maxOffs : 0;
}

/*
  Returns the index of the last column, which may be at the left edge
  of the view.

  Depending on the \link setTableFlags() Tbl_scrollLastHCell\endlink flag,
  this may or may not be the last column.

  \sa maxXOffset(), maxRowOffset()
*/

int QtTableView::maxColOffset()
{
    int mx = maxXOffset();
    if (cellW)
        return mx / cellW;
    else
    {
        int xcd = 0, col = 0;
        while (col < nCols && mx > (xcd = cellWidth(col)))
        {
            mx -= xcd;
            col++;
        }
        return col;
    }
}

/*
  Returns the index of the last row, which may be at the top edge of
  the view.
  Depending on the \link setTableFlags() Tbl_scrollLastVCell\endlink flag,
  this may or may not be the last row.

  \sa maxYOffset(), maxColOffset()
*/

int QtTableView::maxRowOffset()
{
    int my = maxYOffset();
    if (cellH)
        return my / cellH;
    else
    {
        int ycd = 0, row = 0;
        while (row < nRows && my > (ycd = cellHeight(row)))
        {
            my -= ycd;
            row++;
        }
        return row;
    }
}

// DEL
void QtTableView::showOrHideScrollBars() { return; }
