/*
	this is from sc3d5 os9 version.
	not currently working in sc3 os x
*/
/*
GUIWindow {
	classvar <>mainScreen;
	var <>name, <>bounds, <>view, <>visible, <>borderPen, <>contentPen;
	
	*new { arg name, bounds, view, visible=true, borderPen, contentPen;
		contentPen = contentPen.asPen;
		contentPen.backColor = contentPen.backColor ? Color.guiGrey;
		borderPen = borderPen ?? {
			 Pen(
				foreColor: Color.black,
				backColor: contentPen.backColor,
				penSize: 2@2,
				textFont: \Chicago,
				textSize: 12,
				textMode: \srcCopy,
				action: \eraseStroke
			);
		};
		^super.newCopyArgs(name.asString, bounds, view, visible, borderPen, contentPen).layoutView;
	}
	layoutView {
		if (view.notNil, { view.layout(this.contentBounds); });
	}
	titleBarBounds {
		var titleBar;
		titleBar = bounds.copy;
		titleBar.bottom = titleBar.top + 20;
		^titleBar
	}
	contentBounds {
		var contentBounds;
		contentBounds = bounds.insetBy(2,2);
		contentBounds.top = bounds.top + 20;
		^contentBounds
	}
	growBox {
		var growBox;
		growBox = bounds.copy;
		growBox.top = growBox.bottom - 12;
		growBox.left = growBox.right - 12;
		^growBox;
	}
	draw {
		var contentBounds;
		if (visible, {
			borderPen.use({
				var titleBar;
				bounds.draw;
				this.growBox.draw;
				titleBar = this.titleBarBounds;
				titleBar.draw;
				TextBox.new(nil, titleBar.insetBy(2,2), name, \center).draw;
			});
			contentBounds = this.contentBounds;
			contentPen.clipRect = contentBounds;
			if (view.notNil, {
				contentPen.use({ view.draw; });
			});
		});
	}
	mouseDown { arg where, event;
		var msg, dragBounds, tracker;
		if (this.titleBarBounds.containsPoint(where), {
			dragBounds = bounds.copy;
			msg = { arg where, event; this.drag(dragBounds,where,event) };
			^MouseTracker(msg,msg);
		});
		if (this.growBox.containsPoint(where), {
			dragBounds = bounds.copy;
			msg = { arg where, event; this.grow(dragBounds,where,event) };
			^MouseTracker(msg,msg);
		});
		if (view.notNil, { ^view.mouseDown(where, event); });
		^nil
	}
	drag { arg dragBounds, where, event;
		var x, y;
		x = dragBounds.left + where.x - event.where.x;
		y = dragBounds.top + where.y - event.where.y;
		x = x.clip(0, event.screen.bounds.width - 8);
		y = y.clip(0, event.screen.bounds.height - 8);
		bounds = dragBounds.moveTo(x, y);
		this.layoutView;
	}
	grow { arg dragBounds, where, event;
		var x, y;
		x = dragBounds.width + where.x - event.where.x;
		y = dragBounds.height + where.y - event.where.y;
		x = x.clip(32, event.screen.bounds.width - 8);
		y = y.clip(32, event.screen.bounds.height - 8);
		bounds = dragBounds.resizeTo(x, y);
		this.layoutView;		
	}
	mouseOver { arg where, event;
		if (view.notNil, { ^view.mouseOver(where, event); });
		^nil
	}
}
*/

