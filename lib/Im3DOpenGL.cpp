#include "Im3D.h"
#include <GL/gl3w.h>

namespace Im3d
{
    void RenderDrawDataOpenGL(const DrawData* drawData)
    {
        for (int n = 0; n < drawData->drawListsCount; ++n)
        {
            const DrawList* cmdList = drawData->drawLists[n];

            // Copy the vertex and index buffers for this list.


            // Set up the clipping rectangle for this draw list.


            // Copy the camera matrices + light info for this draw list.




            for (int cmdIdx = 0; cmdIdx < cmdList->cmdBuffer.Size; ++cmdIdx)
            {
                const DrawCmd* cmd = &cmdList->cmdBuffer[cmdIdx];

                GLenum mode;
                bool hasIndices = true;

                switch (cmd->type)
                {
                case DrawType::Points:
                    mode = GL_POINTS;
                    hasIndices = false;
                    break;
                case DrawType::Lines:
                    mode = GL_LINES;
                    break;
                case DrawType::Triangles:
                    mode = GL_TRIANGLES;
                    break;
                }

                if (hasIndices)
                {
                    glDrawElements(mode, cmd->count, GL_UNSIGNED_INT, (GLvoid*)(intptr_t)(cmd->offset * sizeof(unsigned int)));
                }
                else
                {
                    glDrawArrays(mode, cmd->offset, cmd->count);
                }
            }
        }

    }

}