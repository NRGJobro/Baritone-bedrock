#include "DataDrivenRendererV2.h"

std::vector<DataDrivenRendererV2::ActorData>& DataDrivenRendererV2::getActors() {
    return hat::member_at<std::vector<ActorData>>(this, 0x10);
}
