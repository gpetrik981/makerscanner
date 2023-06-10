/*
* Copyright 2009-2010, Andrew Barry
*
* This file is part of MakerScanner.
*
* MakerScanner is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License (Version 2, June 1991) as published by
* the Free Software Foundation.
*
* MakerScanner is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "CaptureThread.h"

DEFINE_EVENT_TYPE(IMAGE_UPDATE_EVENT)

CaptureThread::CaptureThread(wxFrame *windowIn, cv::VideoCapture *captureIn)
 : wxThread(wxTHREAD_JOINABLE)
{
    capturing = IDLE;
    window = windowIn;
    cvCapture = captureIn;
}

// called on thread quit -- free all memory
void CaptureThread::OnExit()
{

}

// Called when thread is started
void* CaptureThread::Entry()
{
    while (true)
    {
        // check to see if the thread should exit
        if (TestDestroy() == true)
        {
            break;
        }

        if (capturing == CAPTURE)
        {
            // get a new image
            CaptureFrame();
        } else if (capturing == PREVIEW)
        {

            // get a new image and show it on screen
            CaptureFrame();
            SendFrame(imageQueue.back());
        } else if (capturing == IDLE)
        {
            Sleep(10);
        } else if (capturing == STOP)
        {
            break;
        }

        Yield();
    }

    return NULL;
}

void CaptureThread::CaptureFrame()
{
    if (!cvCapture){
        //fail
        return;
    }

    if (imageQueue.size() > 100)
    {
        // stack too big, throw out some data
        imageQueue.pop();
    }

    cv::Mat frame;
    if (cvCapture && cvCapture->isOpened() && cvCapture->read(frame))
        imageQueue.push(frame);
}

cv::Mat CaptureThread::Pop()
{
    if (imageQueue.size() <= 0)
    {
        CaptureFrame();
    }

    if (imageQueue.size() > 0)
    {
        cv::Mat image = imageQueue.front();
        imageQueue.pop();
        return image;
    }

    return cv::Mat();
}

/*
* Flush the stack, allowing the user to make sure s/he gets the most
* up to date image.  Delete all images in the stack.
*/
void CaptureThread::Flush()
{
    CaptureStatus oldCap = capturing;

    capturing = IDLE;

    while (imageQueue.size() > 0)
    {
        imageQueue.pop();

        // since you should never release an image gotten by cvRetrieveFrame,
        // we don't need to release images here.
    }

    capturing = oldCap;
}

// Display the given image on the frame
// Copies the image so it is safe to change it after the function call
void CaptureThread::SendFrame(cv::Mat frame)
{
    if (frame.empty() || frame.channels() != 1 && frame.channels() != 3)
    {
        return;
    }

    IplImage* pDstImg;
    CvSize sz = cvSize(frame.cols, frame.rows);
    pDstImg = cvCreateImage(sz, 8, 3);
    cvZero(pDstImg);
    cv::Mat dest = cv::cvarrToMat(pDstImg);

    // convert the image into a 3 channel image for display on the frame
    if (frame.channels() == 1)
    {
        cv::cvtColor(frame, dest, CV_GRAY2RGB);
    }
    else if (frame.channels() == 3)
    {
        // opencv stores images as BGR instead of RGB so we need to convert
        cv::cvtColor(frame, dest, CV_BGR2RGB);
    }
    else
    {
        // we don't know how to display this image based on its number of channels
        assert(0);
    }

    wxCommandEvent event(IMAGE_UPDATE_EVENT, GetId());

    // send the image in the event
    event.SetClientData(pDstImg);

    // Send the event to the frame!
    window->GetEventHandler()->AddPendingEvent(event);
}
