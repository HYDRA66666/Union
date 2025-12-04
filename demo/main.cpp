#include "pch.h"
#include "Union/iMutexies.h"

using namespace HYDRA15::Union::labourer;

thread_mutex<atomic_mutex> tmux;


int main()
{
    std::unique_lock ul1(tmux);
    std::unique_lock ul2(tmux);
}