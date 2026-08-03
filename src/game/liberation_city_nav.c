#include "liberation_city_nav.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const int dir_dx[4] = { 0, 1, 0, -1 };
static const int dir_dy[4] = { -1, 0, 1, 0 };
static const float dir_yaw[4] = { 0, (float)(M_PI * 0.5), (float)M_PI, (float)(M_PI * 1.5) };

void city_nav_init(CityNavState *nav, int start_x, int start_y, CityDirection dir) {
    memset(nav, 0, sizeof(*nav));
    nav->cell_x = start_x;
    nav->cell_y = start_y;
    nav->facing = dir;
    nav->smooth_x = start_x * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f;
    nav->smooth_y = start_y * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f;
    nav->smooth_yaw = dir_yaw[dir];
}

uint8_t city_nav_get_cell(const CityGridState *grid, int x, int y) {
    if (x < 0 || x >= CITYGRID_WIDTH || y < 0 || y >= CITYGRID_HEIGHT)
        return CITYGRID_CELL_WALL;
    return grid->plane0[y * CITYGRID_WIDTH + x];
}

bool city_nav_is_road(const CityGridState *grid, int x, int y) {
    uint8_t cell = city_nav_get_cell(grid, x, y);
    return cell == CITYGRID_CELL_ROAD || cell == 0x0A || cell == 0x1F;
}

bool city_nav_is_wall(const CityGridState *grid, int x, int y) {
    uint8_t cell = city_nav_get_cell(grid, x, y);
    return cell == CITYGRID_CELL_WALL;
}

bool city_nav_is_building_entrance(const CityGridState *grid, int x, int y) {
    uint8_t cell = city_nav_get_cell(grid, x, y);
    return cell == 0x0A;
}

bool city_nav_can_move_forward(const CityNavState *nav, const CityGridState *grid) {
    if (nav->moving) return false;
    int nx = nav->cell_x + dir_dx[nav->facing];
    int ny = nav->cell_y + dir_dy[nav->facing];
    return !city_nav_is_wall(grid, nx, ny);
}

bool city_nav_can_move_backward(const CityNavState *nav, const CityGridState *grid) {
    if (nav->moving) return false;
    int back = (nav->facing + 2) & 3;
    int nx = nav->cell_x + dir_dx[back];
    int ny = nav->cell_y + dir_dy[back];
    return !city_nav_is_wall(grid, nx, ny);
}

void city_nav_move_forward(CityNavState *nav, const CityGridState *grid) {
    if (!city_nav_can_move_forward(nav, grid)) return;
    nav->moving = true;
    nav->move_progress = 0;
    nav->move_dx = dir_dx[nav->facing];
    nav->move_dy = dir_dy[nav->facing];
}

void city_nav_move_backward(CityNavState *nav, const CityGridState *grid) {
    if (!city_nav_can_move_backward(nav, grid)) return;
    int back = (nav->facing + 2) & 3;
    nav->moving = true;
    nav->move_progress = 0;
    nav->move_dx = dir_dx[back];
    nav->move_dy = dir_dy[back];
}

void city_nav_turn_left(CityNavState *nav) {
    if (nav->moving) return;
    nav->facing = (CityDirection)((nav->facing + 3) & 3);
    nav->smooth_yaw = dir_yaw[nav->facing];
}

void city_nav_turn_right(CityNavState *nav) {
    if (nav->moving) return;
    nav->facing = (CityDirection)((nav->facing + 1) & 3);
    nav->smooth_yaw = dir_yaw[nav->facing];
}

void city_nav_turn_around(CityNavState *nav) {
    if (nav->moving) return;
    nav->facing = (CityDirection)((nav->facing + 2) & 3);
    nav->smooth_yaw = dir_yaw[nav->facing];
}

void city_nav_update(CityNavState *nav, float dt) {
    if (!nav->moving) return;

    nav->move_progress += dt * 4.0f;
    if (nav->move_progress >= 1.0f) {
        nav->cell_x += nav->move_dx;
        nav->cell_y += nav->move_dy;
        nav->smooth_x = nav->cell_x * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f;
        nav->smooth_y = nav->cell_y * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f;
        nav->moving = false;
        nav->move_progress = 0;
    } else {
        float base_x = nav->cell_x * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f;
        float base_y = nav->cell_y * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f;
        nav->smooth_x = base_x + nav->move_dx * CITY_CELL_SIZE * nav->move_progress;
        nav->smooth_y = base_y + nav->move_dy * CITY_CELL_SIZE * nav->move_progress;
    }
}

static bool is_road_feature(uint8_t cell) {
    return cell == 0x21 || cell == 0x22 || cell == 0x23;
}

static void render_ground_quad(Lib3dState *render,
                                float x0, float z0, float x1, float z1,
                                uint32_t color) {
    X3gVertex verts[4] = {
        { (int16_t)x0, 0, (int16_t)z0, 0, {0,0,0,0} },
        { (int16_t)x1, 0, (int16_t)z0, 0, {0,0,0,0} },
        { (int16_t)x1, 0, (int16_t)z1, 0, {0,0,0,0} },
        { (int16_t)x0, 0, (int16_t)z1, 0, {0,0,0,0} },
    };
    X3gPolygon poly;
    memset(&poly, 0, sizeof(poly));
    poly.vertex_count = 4;
    poly.vertex_indices[0] = 0;
    poly.vertex_indices[1] = 1;
    poly.vertex_indices[2] = 2;
    poly.vertex_indices[3] = 3;
    poly.color = (uint16_t)(color & 0x3F);

    X3gObject obj;
    memset(&obj, 0, sizeof(obj));
    obj.vertices = verts;
    obj.vertex_count = 4;
    obj.parsed_polys = &poly;
    obj.polygon_count = 1;

    lib3d_render_object(render, &obj, 0, 0, 0, NULL, 0);
}

static void render_road_feature(Lib3dState *render, float cx, float cz,
                                 uint8_t feature_type) {
    float h, w;
    uint32_t color;
    switch (feature_type) {
        case 0x21: h = 80.0f; w = 8.0f;  color = 0x1A; break; /* lamp post */
        case 0x22: h = 40.0f; w = 16.0f; color = 0x04; break; /* post box */
        case 0x23: h = 80.0f; w = 24.0f; color = 0x04; break; /* phone box */
        default: return;
    }
    float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    float z0 = cz - w * 0.5f, z1 = cz + w * 0.5f;
    X3gVertex verts[8] = {
        { (int16_t)x0, 0,          (int16_t)z0, 0, {0,0,0,0} },
        { (int16_t)x1, 0,          (int16_t)z0, 0, {0,0,0,0} },
        { (int16_t)x1, 0,          (int16_t)z1, 0, {0,0,0,0} },
        { (int16_t)x0, 0,          (int16_t)z1, 0, {0,0,0,0} },
        { (int16_t)x0, (int16_t)-h, (int16_t)z0, 0, {0,0,0,0} },
        { (int16_t)x1, (int16_t)-h, (int16_t)z0, 0, {0,0,0,0} },
        { (int16_t)x1, (int16_t)-h, (int16_t)z1, 0, {0,0,0,0} },
        { (int16_t)x0, (int16_t)-h, (int16_t)z1, 0, {0,0,0,0} },
    };
    X3gPolygon faces[4];
    memset(faces, 0, sizeof(faces));
    int idx[][4] = {{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}};
    for (int f = 0; f < 4; f++) {
        faces[f].vertex_count = 4;
        for (int v = 0; v < 4; v++) faces[f].vertex_indices[v] = idx[f][v];
        faces[f].color = (uint16_t)(color + f);
    }
    X3gObject obj;
    memset(&obj, 0, sizeof(obj));
    obj.vertices = verts;
    obj.vertex_count = 8;
    obj.parsed_polys = faces;
    obj.polygon_count = 4;
    lib3d_render_object(render, &obj, 0, 0, 0, NULL, 0);
}

static void render_wall_quad(Lib3dState *render,
                              float x0, float z0, float x1, float z1,
                              float height, uint32_t color) {
    X3gVertex verts[4] = {
        { (int16_t)x0, 0,               (int16_t)z0, 0, {0,0,0,0} },
        { (int16_t)x1, 0,               (int16_t)z1, 0, {0,0,0,0} },
        { (int16_t)x1, (int16_t)-height, (int16_t)z1, 0, {0,0,0,0} },
        { (int16_t)x0, (int16_t)-height, (int16_t)z0, 0, {0,0,0,0} },
    };
    X3gPolygon poly;
    memset(&poly, 0, sizeof(poly));
    poly.vertex_count = 4;
    poly.vertex_indices[0] = 0;
    poly.vertex_indices[1] = 1;
    poly.vertex_indices[2] = 2;
    poly.vertex_indices[3] = 3;
    poly.color = (uint16_t)(color & 0x3F);

    X3gObject obj;
    memset(&obj, 0, sizeof(obj));
    obj.vertices = verts;
    obj.vertex_count = 4;
    obj.parsed_polys = &poly;
    obj.polygon_count = 1;

    lib3d_render_object(render, &obj, 0, 0, 0, NULL, 0);
}

void city_nav_render(CityNavState *nav, const CityGridState *grid,
                     Lib3dState *render, const X3gFile *city_vectors,
                     const uint32_t *palette, unsigned pal_size) {
    if (!nav || !grid || !render) return;

    lib3d_set_camera(render, nav->smooth_x, CITY_EYE_HEIGHT, nav->smooth_y,
                     nav->smooth_yaw);
    lib3d_clear(render, 0xFF4466AA, 0xFF446644);

    int view_range = 8;
    for (int dy = -view_range; dy <= view_range; dy++) {
        for (int dx = -view_range; dx <= view_range; dx++) {
            int gx = nav->cell_x + dx;
            int gy = nav->cell_y + dy;

            if (gx < 0 || gx >= CITYGRID_WIDTH || gy < 0 || gy >= CITYGRID_HEIGHT)
                continue;

            float wx = gx * CITY_CELL_SIZE;
            float wy = gy * CITY_CELL_SIZE;
            float cs = CITY_CELL_SIZE;
            uint8_t cell = city_nav_get_cell(grid, gx, gy);

            if (city_nav_is_wall(grid, gx, gy)) {
                bool road_n = !city_nav_is_wall(grid, gx, gy - 1);
                bool road_s = !city_nav_is_wall(grid, gx, gy + 1);
                bool road_e = !city_nav_is_wall(grid, gx + 1, gy);
                bool road_w = !city_nav_is_wall(grid, gx - 1, gy);

                uint32_t wall_color = 0x20 + ((gx * 7 + gy * 13) & 0x1F);
                if (road_n) render_wall_quad(render, wx, wy, wx + cs, wy, CITY_WALL_HEIGHT, wall_color);
                if (road_s) render_wall_quad(render, wx + cs, wy + cs, wx, wy + cs, CITY_WALL_HEIGHT, wall_color);
                if (road_e) render_wall_quad(render, wx + cs, wy, wx + cs, wy + cs, CITY_WALL_HEIGHT, wall_color);
                if (road_w) render_wall_quad(render, wx, wy + cs, wx, wy, CITY_WALL_HEIGHT, wall_color);
            } else {
                render_ground_quad(render, wx, wy, wx + cs, wy + cs, 0x0C);
                if (is_road_feature(cell))
                    render_road_feature(render, wx + cs * 0.5f, wy + cs * 0.5f, cell);
            }
        }
    }

    if (city_vectors && city_vectors->object_count > 0) {
        for (int dy = -view_range; dy <= view_range; dy++) {
            for (int dx = -view_range; dx <= view_range; dx++) {
                int gx = nav->cell_x + dx;
                int gy = nav->cell_y + dy;
                if (gx < 0 || gx >= CITYGRID_WIDTH || gy < 0 || gy >= CITYGRID_HEIGHT)
                    continue;
                if (city_nav_is_building_entrance(grid, gx, gy)) {
                    float ox = gx * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f;
                    float oz = gy * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f;
                    unsigned obj_idx = ((unsigned)(gx + gy * 7)) % city_vectors->object_count;
                    lib3d_render_object(render, &city_vectors->objects[obj_idx],
                                       ox, 0, oz, palette, pal_size);
                }
            }
        }
    }
}

static void render_textured_wall(Lib3dState *render, const Lib3dTexture *tex,
                                  float x0, float z0, float x1, float z1,
                                  float height) {
    lib3d_render_textured_quad(render,
        x0, 0, z0,
        x1, 0, z1,
        x1, -height, z1,
        x0, -height, z0,
        tex);
}

void city_nav_render_textured(CityNavState *nav, const CityGridState *grid,
                              Lib3dState *render, const Lib3dTexture *wall_tex,
                              const X3gFile *city_vectors,
                              const uint32_t *palette, unsigned pal_size) {
    if (!nav || !grid || !render) return;

    lib3d_set_camera(render, nav->smooth_x, CITY_EYE_HEIGHT, nav->smooth_y,
                     nav->smooth_yaw);
    lib3d_clear(render, 0xFF4466AA, 0xFF446644);

    int view_range = 8;
    for (int dy = -view_range; dy <= view_range; dy++) {
        for (int dx = -view_range; dx <= view_range; dx++) {
            int gx = nav->cell_x + dx;
            int gy = nav->cell_y + dy;

            if (gx < 0 || gx >= CITYGRID_WIDTH || gy < 0 || gy >= CITYGRID_HEIGHT)
                continue;

            float wx = gx * CITY_CELL_SIZE;
            float wy = gy * CITY_CELL_SIZE;
            float cs = CITY_CELL_SIZE;
            uint8_t cell = city_nav_get_cell(grid, gx, gy);

            if (city_nav_is_wall(grid, gx, gy)) {
                bool road_n = !city_nav_is_wall(grid, gx, gy - 1);
                bool road_s = !city_nav_is_wall(grid, gx, gy + 1);
                bool road_e = !city_nav_is_wall(grid, gx + 1, gy);
                bool road_w = !city_nav_is_wall(grid, gx - 1, gy);

                if (wall_tex) {
                    if (road_n) render_textured_wall(render, wall_tex, wx, wy, wx + cs, wy, CITY_WALL_HEIGHT);
                    if (road_s) render_textured_wall(render, wall_tex, wx + cs, wy + cs, wx, wy + cs, CITY_WALL_HEIGHT);
                    if (road_e) render_textured_wall(render, wall_tex, wx + cs, wy, wx + cs, wy + cs, CITY_WALL_HEIGHT);
                    if (road_w) render_textured_wall(render, wall_tex, wx, wy + cs, wx, wy, CITY_WALL_HEIGHT);
                } else {
                    uint32_t wall_color = 0x20 + ((gx * 7 + gy * 13) & 0x1F);
                    if (road_n) render_wall_quad(render, wx, wy, wx + cs, wy, CITY_WALL_HEIGHT, wall_color);
                    if (road_s) render_wall_quad(render, wx + cs, wy + cs, wx, wy + cs, CITY_WALL_HEIGHT, wall_color);
                    if (road_e) render_wall_quad(render, wx + cs, wy, wx + cs, wy + cs, CITY_WALL_HEIGHT, wall_color);
                    if (road_w) render_wall_quad(render, wx, wy + cs, wx, wy, CITY_WALL_HEIGHT, wall_color);
                }
            } else {
                render_ground_quad(render, wx, wy, wx + cs, wy + cs, 0x0C);
                if (is_road_feature(cell))
                    render_road_feature(render, wx + cs * 0.5f, wy + cs * 0.5f, cell);
            }
        }
    }

    if (city_vectors && city_vectors->object_count > 0) {
        for (int dy = -view_range; dy <= view_range; dy++) {
            for (int dx = -view_range; dx <= view_range; dx++) {
                int gx = nav->cell_x + dx;
                int gy = nav->cell_y + dy;
                if (gx < 0 || gx >= CITYGRID_WIDTH || gy < 0 || gy >= CITYGRID_HEIGHT)
                    continue;
                if (city_nav_is_building_entrance(grid, gx, gy)) {
                    float ox = gx * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f;
                    float oz = gy * CITY_CELL_SIZE + CITY_CELL_SIZE * 0.5f;
                    unsigned obj_idx = ((unsigned)(gx + gy * 7)) % city_vectors->object_count;
                    lib3d_render_object(render, &city_vectors->objects[obj_idx],
                                       ox, 0, oz, palette, pal_size);
                }
            }
        }
    }
}
