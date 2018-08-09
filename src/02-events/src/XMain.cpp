#include "CrossWindow/CrossWindow.h"
#include "Renderer.h"

void xmain(int argc, const char** argv)
{
    // 🖼️ Create a window
    xwin::EventQueue eventQueue;
    xwin::Window window;

    xwin::WindowDesc windowDesc;
    windowDesc.name = "MainWindow";
    windowDesc.title = "Hello Triangle";
    windowDesc.visible = true;
    windowDesc.width = 1280;
    windowDesc.height = 720;
    //windowDesc.fullscreen = true;
    window.create(windowDesc, eventQueue);

    // 🏁 Engine loop
    bool isRunning = true;
    while (isRunning)
    {
        // ♻️ Update the event queue
        eventQueue.update();

        // 🎈 Iterate through that queue:
        while (!eventQueue.empty())
        {
            //Update Events
            const xwin::Event& event = eventQueue.front();

            // Mouse Input
            if (event.type == xwin::EventType::MouseInput)
            {
                const xwin::MouseInputData data = event.data.mouseInput;
            }

            // Mouse Wheel
            if (event.type == xwin::EventType::MouseWheel)
            {

            }

            // Mouse Movement
            if (event.type == xwin::EventType::MouseMoved)
            {
                const xwin::MouseInputData data = event.data.mouseInput;
            }

            // Resize
            if (event.type == xwin::EventType::Resize)
            {
                const xwin::ResizeData data = event.data.resize;
            }

            // Close
            if (event.type == xwin::EventType::Close)
            {
                window.close();
            }

            eventQueue.pop();
        }
    }

}