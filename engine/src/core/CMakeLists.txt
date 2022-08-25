project(KohiCore)
add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME} PRIVATE logger.c application.c kstring.c event.c input.c clock.c)
