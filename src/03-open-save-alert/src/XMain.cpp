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
                if (data.state == xwin::ButtonState::Released)
                {
                  if (data.button == xwin::MouseInput::Left)
                  {
                    // 💾 Open File Save Dialog
                  }

                  if (data.button == xwin::MouseInput::Right)
                  {
                    // 📂 Open File Open Dialog
                  }

                  if (data.button == xwin::MouseInput::Middle)
                  {
                    // ❗ Open Alert
                  }
                }
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