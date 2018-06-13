#include "CrossWindow/CrossWindow.h"
#include "Renderer.h"

void xmain(int argc, const char** argv)
{
    xwin::EventQueue eventQueue;
    xwin::Window window;

    xwin::WindowDesc windowDesc;
    windowDesc.name = "MainWindow";
    windowDesc.title = "Hello Triangle";
    windowDesc.visible = true;
    windowDesc.width = 1280;
    windowDesc.height = 720;
    window.create(windowDesc, eventQueue);

    Renderer renderer(window);

    // 🏁 Engine loop
    bool isRunning = true;
    while (isRunning)
    {
        // Update Visuals
        renderer.render();

        // ♻️ Update the event queue
        eventQueue.update();

        // 🎈 Iterate through that queue:
        while (!eventQueue.empty())
        {
            //Update Events
            const xwin::Event& event = eventQueue.front();

            switch (event.type)
            {
            case xwin::EventType::Paint:
            {

                break;
            }
            case xwin::EventType::Resize:
            {
                const xwin::ResizeData data = event.data.resize;
                renderer.resize(data.width, data.height);
                break;
            }
            case xwin::EventType::Close:
            {
                window.close();
                isRunning = false;
                break;
            }
            default:
            {
                // Do nothing
                break;
            }
            }

            eventQueue.pop();
        }
    }

}