#ifndef ROAD_H
#define ROAD_H

class Road {
public:
    // Road geometry constants (metres)
    static constexpr float ROAD_HALF_W = 13.5f;  // half-width of asphalt (-13.5 to +13.5)
    static constexpr float CURB_H      = 0.54f;  // kerb height
    static constexpr float SIDEWALK_W  = 12.0f;  // sidewalk width (each side)
    static constexpr float Z_NEAR      = 90.0f;  // road end behind player
    static constexpr float Z_FAR       = -1200.0f;// road end ahead

    // Building wall constants — match main.cpp shop placement
    static constexpr float BLDG_X       = 35.1f;  // centre X of building block
    static constexpr float BLDG_DEPTH   = 19.2f;  // total road-perpendicular depth
    static constexpr float BLDG_H       = 12.0f;  // building height

    void render();

private:
    void drawSurface();
    void drawCurbs();
    void drawSidewalks();
    void drawMarkings();
    void drawBuildingBackwalls();  // solid back/side walls for 3D depth
};

#endif // ROAD_H
