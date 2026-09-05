#pragma once

namespace pixel_world {

class SceneBase {
public:
    virtual ~SceneBase();
    virtual int run() = 0;
};

}  // namespace pixel_world
