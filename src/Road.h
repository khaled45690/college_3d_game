#ifndef ROAD_H
#define ROAD_H

class Road {
public:
    // Road geometry constants (metres)
    static constexpr float ROAD_HALF_W = 4.5f;   // half-width of asphalt (-4.5 to +4.5)
    static constexpr float CURB_H      = 0.18f;  // kerb height
    static constexpr float SIDEWALK_W  = 4.0f;   // sidewalk width (each side)
    static constexpr float Z_NEAR      = 30.0f;  // road end behind player
    static constexpr float Z_FAR       = -400.0f;// road end ahead

    // Building wall constants — match main.cpp shop placement
    static constexpr float BLDG_X       = 11.7f;  // centre X of building block
    static constexpr float BLDG_DEPTH   = 6.4f;   // total road-perpendicular depth
    static constexpr float BLDG_H       = 4.0f;   // building height (lowered)

    void render();

private:
    void drawSurface();
    void drawCurbs();
    void drawSidewalks();
    void drawMarkings();
    void drawBuildingBackwalls();  // solid back/side walls for 3D depth
};

#endif // ROAD_H
