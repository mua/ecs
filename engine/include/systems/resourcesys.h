#ifndef resourcesys_h__
#define resourcesys_h__

#include "systems/system.h"
#include <vector>
#include <string>
#include "components/render.h"

class ResourceSys: public System
{
  private:
	static ResourceSys* _instance;

public:
    GLPipeline createPipeline(Pipeline info);
    RenderPassInstance createRenderPass(RenderPass info);
    ResourceSys();
	~ResourceSys();
	
	void start() override;
	void process() override;

    static std::vector<std::string> searchPaths;
    static std::string absPath(std::string path);	

	static ResourceSys* instance();    
};
#endif // resourcesys_h__
