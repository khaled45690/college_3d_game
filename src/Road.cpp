#include "Road.h"
#include <GL/gl.h>
#include <initializer_list>

// ─── shorthand aliases ───────────────────────────────────────────────────────
static constexpr float RHW = Road::ROAD_HALF_W;
static constexpr float CH  = Road::CURB_H;
static constexpr float SWO = RHW + Road::SIDEWALK_W;   // sidewalk outer X (8.5)
static constexpr float ZN  = Road::Z_NEAR;
static constexpr float ZF  = Road::Z_FAR;

void Road::render() {
    glDisable(GL_TEXTURE_2D);
    drawSurface();
    drawCurbs();
    drawSidewalks();
    drawMarkings();
    drawBuildingBackwalls();

}

// ─── asphalt surface ─────────────────────────────────────────────────────────
void Road::drawSurface() {
    glColor3f(0.17f, 0.17f, 0.17f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-RHW, 0.0f, ZN);
    glVertex3f( RHW, 0.0f, ZN);
    glVertex3f( RHW, 0.0f, ZF);
    glVertex3f(-RHW, 0.0f, ZF);
    glEnd();
}

// ─── kerb vertical faces (face inward toward road) ───────────────────────────
void Road::drawCurbs() {
    glColor3f(0.72f, 0.70f, 0.66f);
    glBegin(GL_QUADS);
    // Left kerb face — normal points +X (toward road)
    glNormal3f(1, 0, 0);
    glVertex3f(-RHW, 0.0f, ZN);
    glVertex3f(-RHW, 0.0f, ZF);
    glVertex3f(-RHW, CH,   ZF);
    glVertex3f(-RHW, CH,   ZN);
    // Right kerb face — normal points -X (toward road)
    glNormal3f(-1, 0, 0);
    glVertex3f(RHW, 0.0f, ZF);
    glVertex3f(RHW, 0.0f, ZN);
    glVertex3f(RHW, CH,   ZN);
    glVertex3f(RHW, CH,   ZF);
    glEnd();
}

// ─── sidewalk top + outer drop ───────────────────────────────────────────────
void Road::drawSidewalks() {
    // top face
    glColor3f(0.60f, 0.60f, 0.57f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    // left
    glVertex3f(-SWO, CH, ZN);
    glVertex3f(-RHW, CH, ZN);
    glVertex3f(-RHW, CH, ZF);
    glVertex3f(-SWO, CH, ZF);
    // right
    glVertex3f(RHW, CH, ZN);
    glVertex3f(SWO, CH, ZN);
    glVertex3f(SWO, CH, ZF);
    glVertex3f(RHW, CH, ZF);
    glEnd();

    // outer vertical drop
    glColor3f(0.48f, 0.48f, 0.45f);
    glBegin(GL_QUADS);
    glNormal3f(-1, 0, 0);
    glVertex3f(-SWO, CH,   ZN);
    glVertex3f(-SWO, 0.0f, ZN);
    glVertex3f(-SWO, 0.0f, ZF);
    glVertex3f(-SWO, CH,   ZF);
    glNormal3f(1, 0, 0);
    glVertex3f(SWO, CH,   ZN);
    glVertex3f(SWO, CH,   ZF);
    glVertex3f(SWO, 0.0f, ZF);
    glVertex3f(SWO, 0.0f, ZN);
    glEnd();

    // near and far end caps (stop sidewalk looking open at ends)
    glColor3f(0.60f, 0.60f, 0.57f);
    for (float z : {ZN, ZF}) {
        float nz = (z == ZN) ? 1.0f : -1.0f;
        glBegin(GL_QUADS);
        glNormal3f(0, 0, nz);
        glVertex3f(-SWO, 0.0f, z);
        glVertex3f(-RHW, 0.0f, z);
        glVertex3f(-RHW, CH,   z);
        glVertex3f(-SWO, CH,   z);
        glVertex3f( RHW, 0.0f, z);
        glVertex3f( SWO, 0.0f, z);
        glVertex3f( SWO, CH,   z);
        glVertex3f( RHW, CH,   z);
        glEnd();
    }
}

// ─── lane markings ───────────────────────────────────────────────────────────
void Road::drawMarkings() {
    const float y   = 0.036f;  // just above surface to avoid z-fighting
    const float hw  = 0.30f;   // half-width of line → 60 cm total
    const float dLen = 9.0f, gap = 6.0f, period = dLen + gap;

    glNormal3f(0, 1, 0);

    // ── solid white shoulder lines (full length) ──────────────────────────
    glColor3f(0.95f, 0.95f, 0.95f);
    const float shX = 0.45f;   // inset from kerb
    glBegin(GL_QUADS);
    // left shoulder
    glVertex3f(-RHW + shX,        y, ZN);
    glVertex3f(-RHW + shX + 2*hw, y, ZN);
    glVertex3f(-RHW + shX + 2*hw, y, ZF);
    glVertex3f(-RHW + shX,        y, ZF);
    // right shoulder
    glVertex3f(RHW - shX - 2*hw, y, ZN);
    glVertex3f(RHW - shX,        y, ZN);
    glVertex3f(RHW - shX,        y, ZF);
    glVertex3f(RHW - shX - 2*hw, y, ZF);
    glEnd();

    // ── dashed dividers and centre ────────────────────────────────────────
    for (float z = ZN; z > ZF; z -= period) {
        float ze = z - dLen;
        if (ze < ZF) ze = ZF;

        // white lane dividers at X = ±4.5
        glColor3f(0.95f, 0.95f, 0.95f);
        float lx[2] = {-4.5f, 4.5f};
        for (int i = 0; i < 2; ++i) {
            glBegin(GL_QUADS);
            glNormal3f(0, 1, 0);
            glVertex3f(lx[i]-hw, y, z);
            glVertex3f(lx[i]+hw, y, z);
            glVertex3f(lx[i]+hw, y, ze);
            glVertex3f(lx[i]-hw, y, ze);
            glEnd();
        }

        // yellow centre line at X = 0
        glColor3f(1.0f, 0.82f, 0.0f);
        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glVertex3f(-hw, y, z);
        glVertex3f( hw, y, z);
        glVertex3f( hw, y, ze);
        glVertex3f(-hw, y, ze);
        glEnd();
    }
}

// ─── building back-walls & top cap (give shop blocks visible 3D depth) ───────
void Road::drawBuildingBackwalls() {
    const float bx   = BLDG_X;
    const float half = BLDG_DEPTH * 0.5f;
    const float h    = 0.3f;
    // back wall X position (away from road)
    const float bxOuter = bx + half;
    const float bxInner = bx - half;   // road-facing side, just past sidewalk

    // ── top cap (roof) ────────────────────────────────────────────────────
    glColor3f(0.45f, 0.42f, 0.38f);   // dark warm grey roof
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    // right roof
    glVertex3f(bxInner, h, ZN);
    glVertex3f(bxOuter, h, ZN);
    glVertex3f(bxOuter, h, ZF);
    glVertex3f(bxInner, h, ZF);
    // left roof
    glVertex3f(-bxOuter, h, ZN);
    glVertex3f(-bxInner, h, ZN);
    glVertex3f(-bxInner, h, ZF);
    glVertex3f(-bxOuter, h, ZF);
    glEnd();

    // ── front wall (road-facing side at bxInner, visible from road) ──────
    glColor3f(0.42f, 0.38f, 0.30f);
    glBegin(GL_QUADS);
    // right front wall — normal -X (faces toward road centre)
    glNormal3f(-1, 0, 0);
    glVertex3f(bxInner, 0.0f, ZF);
    glVertex3f(bxInner, h,    ZF);
    glVertex3f(bxInner, h,    ZN);
    glVertex3f(bxInner, 0.0f, ZN);
    // left front wall — normal +X (faces toward road centre)
    glNormal3f(1, 0, 0);
    glVertex3f(-bxInner, 0.0f, ZN);
    glVertex3f(-bxInner, h,    ZN);
    glVertex3f(-bxInner, h,    ZF);
    glVertex3f(-bxInner, 0.0f, ZF);
    glEnd();
}
