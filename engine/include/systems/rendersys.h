#ifndef RenderSys_h__
#define RenderSys_h__

#include "System.h"
#include "Types.h"
#include "components/render.h"

class RenderSys : public System
{
  private:
    static RenderSys *_instance;
    void bindMaterial(const Geometry &geo, const GLPipeline &pipeline);

	void drawGeometry(const GLPipeline &pipeline, const GLGeometry &ggeo);

	void renderToTarget(GLFrameBuffer &target, GLPipeline& pipeline, Camera &camera,
                        std::vector<GLAttachment> &attachments);

	void renderPass(RenderPassInstance &pass, Camera &camera);

  public:
    RenderSys();
    ~RenderSys();

    void start() override;
    void process() override;

    static void *uiRenderTargetHandle(Handle entity);
    static RenderSys *instance();
    

  private:

};


#endif // RenderSys_h__
