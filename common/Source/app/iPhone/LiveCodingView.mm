//
//  LiveCodingView.m
//  isclang
//
//  Created by Axel Balley on 30/10/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "LiveCodingView.h"

extern void rtf2txt(char *txt);

@implementation LiveCodingView


- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame])
	{
		target = 0;
		selector = 0;
    }
    return self;
}

- (void) awakeFromNib
{
	[textView setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	[textView setAutocorrectionType:UITextAutocorrectionTypeNo];
	[self showButtons:NO];
}

- (void)drawRect:(CGRect)rect {
    [super drawRect:rect];
}

- (void) setTarget:(id)t withSelector:(SEL)s
{
	target = t;
	selector = s;
}

- (void) loadFile:(NSString *)file
{
	NSString *contents = [NSString stringWithContentsOfFile:file encoding:NSASCIIStringEncoding error:nil];
	int length = [contents length];
	char *buf = (char *) malloc(length+1);
	[contents getCString:buf maxLength:length+1 encoding:NSASCIIStringEncoding];
	rtf2txt(buf);
	while (*buf=='\n') buf++;
	
	[textView setText:[NSString stringWithCString:buf encoding:NSASCIIStringEncoding]];
}

- (void) showButtons: (BOOL)state
{
	[doneButton setHidden:!state];
	[lineButton setHidden:!state];
	[blockButton setHidden:!state];
}

- (void) textViewDidBeginEditing: (UITextView *)theView
{
	[self showButtons:YES];
}

- (IBAction) triggerDone: (id)sender
{
	[textView resignFirstResponder];
	[self showButtons:NO];
}

- (IBAction) triggerExecute: (id)sender
{
	NSRange range = [textView selectedRange];
	NSString *text = [textView text];
	NSUInteger start, end;
	[text getLineStart:&start end:&end contentsEnd:nil forRange:range];
	NSString *line = [text substringWithRange:NSMakeRange(start, end-start)];
	
	if (target && [target respondsToSelector:selector]) [target performSelector:selector withObject:line];
	
	[textView resignFirstResponder];
	[self showButtons:NO];
}

- (IBAction) triggerExecuteFile: (id)sender
{
	if (target && [target respondsToSelector:selector]) [target performSelector:selector withObject:[textView text]];
}

- (void)dealloc {
    [super dealloc];
}


@end
