#include<cstdlib>
#include<SDL3/SDL.h>

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO))
    abort();

  while (true) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        goto exit;
        break;

      case SDL_EVENT_KEY_DOWN:
        if (event.key.scancode == SDL_SCANCODE_ESCAPE)
          goto exit;
        break;
        
      default:
        break;
      }
    }
  }

exit:
  SDL_Quit();

  return 0;
}
