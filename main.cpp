/*
 * ================================================================
 *  BMW E46 3-Series (2005) — Raw OpenGL Mesh + Environment Map
 *  ────────────────────────────────────────────────────────────
 *  NO glutSolidSphere / glutSolidCube — every surface is built
 *  from hand‑crafted vertex arrays with smooth normals and
 *  sphere‑mapped environment reflections for realistic paint.
 *
 *  Body is generated parametrically from shape functions that
 *  vary along the car's length, producing ~2400 smooth quads.
 *
 *  Compile (Linux):
 *    gcc bmw_e46_v2.c -o bmw -lGL -lGLU -lglut -lm
 *  Compile (macOS):
 *    gcc bmw_e46_v2.c -o bmw -framework OpenGL -framework GLUT -lm
 *  Compile (Windows / MinGW + FreeGLUT):
 *    gcc bmw_e46_v2.c -o bmw.exe -lfreeglut -lopengl32 -lglu32 -lm
 *
 *  Controls:
 *    SPACE      turntable on / off
 *    1‑5        paint colour
 *    W/S        zoom      A/D  pan
 *    Arrows     pitch / yaw
 *    Mouse      orbit     Scroll  zoom
 *    +/−        turntable speed
 *    R          reset     ESC  quit
 * ================================================================
 */

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define RAD(d) ((d)*M_PI/180.0)

/* guard missing constant */
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

/* ──────────── vector helpers ──────────── */
typedef struct { float x,y,z; } V3;

static V3 v3(float x,float y,float z){ V3 v={x,y,z}; return v; }
static V3 v3sub(V3 a,V3 b){ return v3(a.x-b.x, a.y-b.y, a.z-b.z); }
static V3 v3cross(V3 a,V3 b){ return v3(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x); }
static V3 v3norm(V3 v){
    float l=sqrtf(v.x*v.x+v.y*v.y+v.z*v.z);
    if(l<1e-8f) return v3(0,1,0);
    return v3(v.x/l, v.y/l, v.z/l);
}
static V3 v3add(V3 a,V3 b){ return v3(a.x+b.x, a.y+b.y, a.z+b.z); }
static V3 v3scale(V3 a,float s){ return v3(a.x*s, a.y*s, a.z*s); }

/* ─────────────────── camera ─────────────────── */
static float camYaw=25, camPitch=-8, camDist=7.0f;
static float camPanX=0, camPanY=0;
static int mouseDown=0, lastMX, lastMY;

/* ─────────────────── turntable ───────────────── */
static int  autoRot=1;
static float turnAngle=0, turnSpd=0.30f;

/* ─────────────────── paint ──────────────────── */
typedef struct { float r,g,b; const char *name; } Paint;
static Paint paints[] = {
    {0.72f,0.73f,0.76f, "Titanium Silver"},
    {0.03f,0.03f,0.04f, "Jet Black"},
    {0.06f,0.15f,0.38f, "Orient Blue"},
    {0.58f,0.06f,0.04f, "Imola Red"},
    {0.93f,0.93f,0.92f, "Alpine White"},
};
static int curPaint=0;

/* ─────────────────── env‑map texture ────────── */
static GLuint envTex;

/* ═══════════════════════════════════════════════
 *  PROCEDURAL ENVIRONMENT MAP
 *  Horizontal light bands simulating a studio
 *  soft‑box setup — gives the classic car‑ad
 *  reflections across body panels.
 * ═══════════════════════════════════════════════ */
static void generateEnvMap(void)
{
    #define ENV_SZ 256
    unsigned char data[ENV_SZ][ENV_SZ][3];
    int x, y;
    for (y = 0; y < ENV_SZ; y++) {
        float ny = (y - ENV_SZ/2.0f) / (ENV_SZ/2.0f);   /* −1..1 */
        for (x = 0; x < ENV_SZ; x++) {
            float nx = (x - ENV_SZ/2.0f) / (ENV_SZ/2.0f);

            /* base: dark studio */
            float b = 0.08f;

            /* horizon glow */
            b += 0.18f * expf(-ny*ny * 6.0f);

            /* two softbox bands */
            b += 0.72f * expf(-(ny-0.32f)*(ny-0.32f) * 55.0f);
            b += 0.50f * expf(-(ny+0.25f)*(ny+0.25f) * 45.0f);

            /* narrow overhead strip */
            b += 0.35f * expf(-(ny-0.75f)*(ny-0.75f) * 120.0f);

            /* slight horizontal variation */
            b += 0.04f * sinf(nx * (float)M_PI * 3.0f);

            /* gentle vertical gradient for ground reflection */
            if (ny < -0.5f) {
                float t = (-0.5f - ny) / 0.5f;
                b += 0.10f * t;
            }

            if (b > 1.0f) b = 1.0f;

            /* slightly warm tint */
            data[y][x][0] = (unsigned char)(b * 248);
            data[y][x][1] = (unsigned char)(b * 250);
            data[y][x][2] = (unsigned char)(b * 255);
        }
    }

    glGenTextures(1, &envTex);
    glBindTexture(GL_TEXTURE_2D, envTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ENV_SZ, ENV_SZ, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, data);
    #undef ENV_SZ
}

/* ═══════════════════════════════════════════════
 *  PARAMETRIC BODY SHAPE
 *  Functions of z (front = +2.2, rear = −2.2)
 *  return cross‑section parameters.
 * ═══════════════════════════════════════════════ */

/* smoothstep */
static float ss(float e0, float e1, float x){
    float t=(x-e0)/(e1-e0);
    if(t<0) t=0; if(t>1) t=1;
    return t*t*(3-2*t);
}
static float lerp(float a,float b,float t){ return a+(b-a)*t; }

/* half‑width of the body (max X) */
static float bodyHW(float z)
{
    float base = 0.88f;
    /* front taper (nose) */
    if (z > 1.55f) {
        float t = ss(1.55f, 2.25f, z);
        base = lerp(0.88f, 0.62f, t);
    }
    /* rear taper */
    if (z < -1.35f) {
        float t = ss(-1.35f, -2.20f, z);
        base = lerp(0.88f, 0.66f, t);
    }
    /* rear fender flare */
    if (z > -1.50f && z < -0.40f) {
        float mid = -0.95f;
        float d = (z - mid) / 0.55f;
        base += 0.035f * expf(-d*d * 2.0f);
    }
    /* front fender flare (subtle) */
    if (z > 0.50f && z < 1.55f) {
        float mid = 1.02f;
        float d = (z - mid) / 0.52f;
        base += 0.025f * expf(-d*d * 2.0f);
    }
    return base;
}

/* body panel height (top of fender / beltline) */
static float beltH(float z)
{
    float base = 0.90f;
    if (z > 1.60f) {
        float t = ss(1.60f, 2.20f, z);
        base = lerp(0.90f, 0.60f, t);
    }
    if (z < -1.30f) {
        float t = ss(-1.30f, -2.15f, z);
        base = lerp(0.90f, 0.72f, t);
    }
    return base;
}

/* roof half‑width */
static float roofHW(float z)
{
    float base = 0.60f;
    /* roof exists roughly z ∈ [−0.9, 0.45] */
    if (z > 0.45f || z < -0.90f) return 0.0f;  /* no roof outside greenhouse */
    /* taper at front (windshield) */
    if (z > 0.10f) {
        float t = ss(0.10f, 0.45f, z);
        base = lerp(0.60f, 0.50f, t);
    }
    /* taper at rear (C‑pillar) */
    if (z < -0.55f) {
        float t = ss(-0.55f, -0.90f, z);
        base = lerp(0.60f, 0.42f, t);
    }
    return base;
}

/* roof height */
static float roofH(float z)
{
    float base = 1.38f;
    if (z > 0.45f || z < -0.90f) return beltH(z);
    /* slight dome */
    float mid = -0.22f;
    float d = (z - mid) / 0.68f;
    base += 0.02f * expf(-d*d);
    /* lower at front */
    if (z > 0.10f) {
        float t = ss(0.10f, 0.45f, z);
        base = lerp(1.38f, 1.18f, t);
    }
    /* lower at rear */
    if (z < -0.55f) {
        float t = ss(-0.55f, -0.90f, z);
        base = lerp(1.38f, 1.15f, t);
    }
    return base;
}

/* floor half‑width (under body) */
static float floorHW(float z)
{
    float bw = bodyHW(z);
    return bw * 0.82f;
}

/* lower body height (sill line) */
static float sillH(float z)
{
    return 0.20f;
}

/* ═══════════════════════════════════════════════
 *  BODY MESH GENERATION
 *  Sample the parametric shape at NZ stations,
 *  NPP points per profile, store vertices + normals.
 * ═══════════════════════════════════════════════ */

#define NZ  52          /* stations along length */
#define NPP 28          /* points per profile (full loop) */
#define HALF (NPP/2)    /* points in right half */

static V3 bodyV[NZ][NPP];   /* vertices */
static V3 bodyN[NZ][NPP];   /* normals  */
static float zStations[NZ];

/* Build profile at z: right half from bottom‑centre going
   up to roof‑centre, then mirror for left half. */
static void buildProfile(float z, V3 *pts)
{
    float bw  = bodyHW(z);
    float sl  = sillH(z);
    float bl  = beltH(z);
    float rw  = roofHW(z);
    float rh  = roofH(z);
    float fw  = floorHW(z);
    int hasRoof = (rw > 0.01f);

    /* ── right half (indices 0..HALF−1) ── */
    /* 0: bottom centre */
    pts[0] = v3(0.0f, 0.0f, z);

    /* 1: floor edge */
    pts[1] = v3(fw, 0.0f, z);

    /* 2: lower body corner (rounded) */
    pts[2] = v3(bw - 0.02f, 0.04f, z);

    /* 3: sill */
    pts[3] = v3(bw, sl, z);

    /* 4: lower body mid */
    pts[4] = v3(bw + 0.005f, (sl + bl) * 0.38f, z);

    /* 5: character line */
    pts[5] = v3(bw + 0.008f, (sl + bl) * 0.52f, z);

    /* 6: upper body */
    pts[6] = v3(bw, (sl + bl) * 0.72f, z);

    /* 7: shoulder */
    pts[7] = v3(bw - 0.01f, bl - 0.06f, z);

    /* 8: beltline */
    pts[8] = v3(bw - 0.03f, bl, z);

    /* 9‑12: greenhouse / roof transition */
    if (hasRoof) {
        float glassX = lerp(bw - 0.06f, rw + 0.04f, 0.3f);
        pts[9]  = v3(glassX,  bl + 0.08f, z);
        pts[10] = v3(rw + 0.02f, rh - 0.12f, z);
        pts[11] = v3(rw, rh - 0.03f, z);
        pts[12] = v3(rw * 0.55f, rh, z);
        pts[13] = v3(0.0f, rh + 0.01f, z);  /* roof centre */
    } else {
        /* no roof — smooth from beltline to centre at same height */
        float topY = bl + 0.02f;
        pts[9]  = v3(bw * 0.75f, topY, z);
        pts[10] = v3(bw * 0.50f, topY + 0.01f, z);
        pts[11] = v3(bw * 0.30f, topY + 0.01f, z);
        pts[12] = v3(bw * 0.12f, topY + 0.005f, z);
        pts[13] = v3(0.0f, topY, z);
    }

    /* ── left half: mirror indices 12→1 into 14..NPP−1 ── */
    int src, dst;
    for (src = 12, dst = 14; src >= 1; src--, dst++) {
        pts[dst] = pts[src];
        pts[dst].x = -pts[dst].x;
    }
    /* dst should reach NPP (28). 14 + 12 = 26... we have indices 14..25 = 12 pts.
       But HALF=14 → right half is 0..13, left mirror is 14..27 = 14 pts mirroring 12..0
       Let me redo: mirror 12 down to 0 = 13 values → 14..26 = 13 slots.
       Index 27 wraps to 0 (bottom centre). */

    /* Actually let me just recalculate. Right half: 0..13 (14 points).
       Left half mirrors right‑side interior points (12 down to 1) = 12 points → indices 14..25.
       Total used = 26. NPP=28, so pad the last two as interpolation back to bottom.
       OR: just use NPP=26. Let me redefine. */

    /* Fix: re-mirror properly */
    /* Right: 0(bot ctr) .. 13(top ctr) = 14 pts
       Left:  14..25 = mirror of 12..1  = 12 pts
       Close: 26 = copy of 0 (close the loop) — but we use mod NPP.
       So useful pts = 26. Pad 26,27 = same as 0 to close smoothly. */
    /* indices 26, 27 */
    if (NPP > 26) {
        pts[26] = pts[0];
        pts[26].x = -0.001f;  /* just slightly left of centre */
        pts[26].y = 0.01f;
    }
    if (NPP > 27) {
        pts[27] = pts[0]; /* close loop */
    }
}

static void initBodyMesh(void)
{
    int i, j;
    float zMin = -2.22f, zMax = 2.22f;

    /* distribute stations */
    for (i = 0; i < NZ; i++) {
        zStations[i] = zMin + (zMax - zMin) * i / (float)(NZ - 1);
    }

    /* generate profiles */
    for (i = 0; i < NZ; i++) {
        buildProfile(zStations[i], bodyV[i]);
    }

    /* compute smooth normals via central differences */
    for (i = 0; i < NZ; i++) {
        for (j = 0; j < NPP; j++) {
            /* tangent along z (length) */
            int ip = (i < NZ-1) ? i+1 : i;
            int im = (i > 0)    ? i-1 : i;
            V3 dz = v3sub(bodyV[ip][j], bodyV[im][j]);

            /* tangent along profile */
            int jp = (j + 1) % NPP;
            int jm = (j - 1 + NPP) % NPP;
            V3 dp = v3sub(bodyV[i][jp], bodyV[i][jm]);

            /* normal = cross(dz, dp) — outward */
            V3 n = v3norm(v3cross(dz, dp));

            /* flip if pointing inward (check against radial direction) */
            V3 radial = v3(bodyV[i][j].x, bodyV[i][j].y - 0.6f, 0.0f);
            float dot = n.x*radial.x + n.y*radial.y;
            if (dot < 0) { n.x=-n.x; n.y=-n.y; n.z=-n.z; }

            bodyN[i][j] = n;
        }
    }
}

/* ═══════════════════════════════════════════════
 *  MATERIAL HELPERS
 * ═══════════════════════════════════════════════ */
static void setMat(float r, float g, float b, float spec, float shine)
{
    float a[4]={r*0.18f,g*0.18f,b*0.18f,1};
    float d[4]={r,g,b,1};
    float s[4]={spec,spec,spec,1};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  a);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  d);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, s);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, shine);
}

static void setPaint(void)
{
    Paint p = paints[curPaint];
    setMat(p.r, p.g, p.b, 0.85f, 96.0f);
}

/* ═══════════════════════════════════════════════
 *  RENDER BODY with environment‑mapped reflections
 * ═══════════════════════════════════════════════ */
static void renderBody(void)
{
    int i, j;

    /* ── Pass 1: base paint with lighting ── */
    setPaint();
    for (i = 0; i < NZ - 1; i++) {
        glBegin(GL_QUAD_STRIP);
        for (j = 0; j < NPP; j++) {
            glNormal3f(bodyN[i+1][j].x, bodyN[i+1][j].y, bodyN[i+1][j].z);
            glVertex3f(bodyV[i+1][j].x, bodyV[i+1][j].y, bodyV[i+1][j].z);
            glNormal3f(bodyN[i][j].x,   bodyN[i][j].y,   bodyN[i][j].z);
            glVertex3f(bodyV[i][j].x,    bodyV[i][j].y,   bodyV[i][j].z);
        }
        /* close the strip */
        glNormal3f(bodyN[i+1][0].x, bodyN[i+1][0].y, bodyN[i+1][0].z);
        glVertex3f(bodyV[i+1][0].x, bodyV[i+1][0].y, bodyV[i+1][0].z);
        glNormal3f(bodyN[i][0].x,   bodyN[i][0].y,   bodyN[i][0].z);
        glVertex3f(bodyV[i][0].x,    bodyV[i][0].y,   bodyV[i][0].z);
        glEnd();
    }

    /* ── Pass 2: additive environment reflection ── */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, envTex);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    /* blend: add reflection on top of base */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthFunc(GL_EQUAL);    /* only draw where body already is */

    /* dim white material so reflection strength is controllable */
    float reflStr = 0.30f;
    /* darker paints reflect more visibly */
    Paint p = paints[curPaint];
    float lum = p.r * 0.299f + p.g * 0.587f + p.b * 0.114f;
    reflStr = lerp(0.38f, 0.18f, lum);  /* more reflection on dark paint */

    setMat(reflStr, reflStr, reflStr, 0.0f, 1.0f);
    glDisable(GL_LIGHTING);
    glColor3f(reflStr, reflStr, reflStr);

    for (i = 0; i < NZ - 1; i++) {
        glBegin(GL_QUAD_STRIP);
        for (j = 0; j < NPP; j++) {
            glNormal3f(bodyN[i+1][j].x, bodyN[i+1][j].y, bodyN[i+1][j].z);
            glVertex3f(bodyV[i+1][j].x, bodyV[i+1][j].y, bodyV[i+1][j].z);
            glNormal3f(bodyN[i][j].x,   bodyN[i][j].y,   bodyN[i][j].z);
            glVertex3f(bodyV[i][j].x,    bodyV[i][j].y,   bodyV[i][j].z);
        }
        glNormal3f(bodyN[i+1][0].x, bodyN[i+1][0].y, bodyN[i+1][0].z);
        glVertex3f(bodyV[i+1][0].x, bodyV[i+1][0].y, bodyV[i+1][0].z);
        glNormal3f(bodyN[i][0].x,   bodyN[i][0].y,   bodyN[i][0].z);
        glVertex3f(bodyV[i][0].x,    bodyV[i][0].y,   bodyV[i][0].z);
        glEnd();
    }

    glEnable(GL_LIGHTING);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);
}

/* ═══════════════════════════════════════════════
 *  PARAMETRIC TORUS (for tyres and rings)
 * ═══════════════════════════════════════════════ */
static void renderTorus(float R, float r, int ns, int nr)
{
    int i, j;
    for (i = 0; i < ns; i++) {
        float t0 = (float)i / ns * 2.0f * (float)M_PI;
        float t1 = (float)(i+1) / ns * 2.0f * (float)M_PI;
        glBegin(GL_QUAD_STRIP);
        for (j = 0; j <= nr; j++) {
            float p = (float)j / nr * 2.0f * (float)M_PI;
            float cp = cosf(p), sp = sinf(p);
            float ct0 = cosf(t0), st0 = sinf(t0);
            float ct1 = cosf(t1), st1 = sinf(t1);

            float x0 = (R + r*cp)*ct0, y0 = r*sp, z0 = (R + r*cp)*st0;
            float x1 = (R + r*cp)*ct1, y1 = r*sp, z1 = (R + r*cp)*st1;
            V3 n0 = v3norm(v3(cp*ct0, sp, cp*st0));
            V3 n1 = v3norm(v3(cp*ct1, sp, cp*st1));

            glNormal3f(n0.x,n0.y,n0.z); glVertex3f(x0,y0,z0);
            glNormal3f(n1.x,n1.y,n1.z); glVertex3f(x1,y1,z1);
        }
        glEnd();
    }
}

/* ═══════════════════════════════════════════════
 *  PARAMETRIC CYLINDER (along Y axis)
 * ═══════════════════════════════════════════════ */
static void renderCylinder(float r, float h, int sl)
{
    int i;
    float step = 2.0f*(float)M_PI/sl;
    /* side */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= sl; i++) {
        float a = i * step;
        float ca = cosf(a), sa = sinf(a);
        glNormal3f(ca, 0, sa);
        glVertex3f(r*ca, 0,   r*sa);
        glVertex3f(r*ca, h, r*sa);
    }
    glEnd();
    /* caps */
    glNormal3f(0,-1,0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0,0,0);
    for(i=0;i<=sl;i++){float a=i*step; glVertex3f(r*cosf(a),0,r*sinf(a));}
    glEnd();
    glNormal3f(0,1,0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0,h,0);
    for(i=sl;i>=0;i--){float a=i*step; glVertex3f(r*cosf(a),h,r*sinf(a));}
    glEnd();
}

/* ═══════════════════════════════════════════════
 *  PARAMETRIC DISC (flat circle in XZ plane at y=0)
 * ═══════════════════════════════════════════════ */
static void renderDisc(float rIn, float rOut, int sl)
{
    int i;
    float step = 2.0f*(float)M_PI/sl;
    glNormal3f(0,1,0);
    glBegin(GL_QUAD_STRIP);
    for(i=0;i<=sl;i++){
        float a=i*step, ca=cosf(a), sa=sinf(a);
        glVertex3f(rIn*ca,  0, rIn*sa);
        glVertex3f(rOut*ca, 0, rOut*sa);
    }
    glEnd();
}

/* ═══════════════════════════════════════════════
 *  WHEEL ASSEMBLY  (tyre + alloy + brake)
 * ═══════════════════════════════════════════════ */
static void drawWheel(int flipSide)
{
    float s = flipSide ? -1.0f : 1.0f;

    glPushMatrix();

    /* tyre */
    setMat(0.06f,0.06f,0.06f, 0.04f, 3.0f);
    glPushMatrix();
        glRotatef(90, 1,0,0);
        renderTorus(0.26f, 0.09f, 32, 16);
    glPopMatrix();

    /* tyre sidewall bulge */
    setMat(0.05f,0.05f,0.05f, 0.02f, 2.0f);
    glPushMatrix();
        glTranslatef(0, 0, s*0.08f);
        glRotatef(90, 1,0,0);
        renderTorus(0.24f, 0.035f, 28, 10);
    glPopMatrix();

    /* outer rim ring */
    setMat(0.82f,0.83f,0.85f, 0.92f, 110.0f);
    glPushMatrix();
        glRotatef(90, 1,0,0);
        renderTorus(0.225f, 0.018f, 32, 10);
    glPopMatrix();

    /* inner rim ring */
    glPushMatrix();
        glRotatef(90, 1,0,0);
        renderTorus(0.10f, 0.015f, 24, 8);
    glPopMatrix();

    /* hub */
    glPushMatrix();
        glTranslatef(0, 0, s*0.08f);
        glRotatef(90, 0,0,1);
        renderCylinder(0.05f, 0.001f, 20);
    glPopMatrix();

    /* 5 spokes + fork splits */
    int i;
    setMat(0.80f,0.81f,0.83f, 0.90f, 105.0f);
    for (i = 0; i < 5; i++) {
        float ang = i * 72.0f;
        /* main spoke */
        glPushMatrix();
            glRotatef(ang, 0,0,1);
            glTranslatef(0.0f, 0.16f, s*0.065f);
            glPushMatrix();
                glScalef(0.018f, 0.14f, 0.012f);
                /* inline box: just use quads */
                glBegin(GL_QUADS);
                    /* front */ glNormal3f(0,0,1);
                    glVertex3f(-0.5f,-0.5f, 0.5f); glVertex3f(0.5f,-0.5f,0.5f);
                    glVertex3f(0.5f,0.5f,0.5f); glVertex3f(-0.5f,0.5f,0.5f);
                    /* back */ glNormal3f(0,0,-1);
                    glVertex3f(0.5f,-0.5f,-0.5f); glVertex3f(-0.5f,-0.5f,-0.5f);
                    glVertex3f(-0.5f,0.5f,-0.5f); glVertex3f(0.5f,0.5f,-0.5f);
                    /* top */ glNormal3f(0,1,0);
                    glVertex3f(-0.5f,0.5f,-0.5f); glVertex3f(-0.5f,0.5f,0.5f);
                    glVertex3f(0.5f,0.5f,0.5f); glVertex3f(0.5f,0.5f,-0.5f);
                    /* bottom */ glNormal3f(0,-1,0);
                    glVertex3f(-0.5f,-0.5f,0.5f); glVertex3f(-0.5f,-0.5f,-0.5f);
                    glVertex3f(0.5f,-0.5f,-0.5f); glVertex3f(0.5f,-0.5f,0.5f);
                    /* right */ glNormal3f(1,0,0);
                    glVertex3f(0.5f,-0.5f,-0.5f); glVertex3f(0.5f,-0.5f,0.5f);
                    glVertex3f(0.5f,0.5f,0.5f); glVertex3f(0.5f,0.5f,-0.5f);
                    /* left */ glNormal3f(-1,0,0);
                    glVertex3f(-0.5f,-0.5f,0.5f); glVertex3f(-0.5f,-0.5f,-0.5f);
                    glVertex3f(-0.5f,0.5f,-0.5f); glVertex3f(-0.5f,0.5f,0.5f);
                glEnd();
            glPopMatrix();
        glPopMatrix();

        /* fork split left */
        glPushMatrix();
            glRotatef(ang + 9.0f, 0,0,1);
            glTranslatef(0.0f, 0.20f, s*0.065f);
            glPushMatrix();
                glScalef(0.010f, 0.055f, 0.008f);
                glBegin(GL_QUADS);
                    glNormal3f(0,0,1); glVertex3f(-0.5f,-0.5f,0.5f); glVertex3f(0.5f,-0.5f,0.5f);
                    glVertex3f(0.5f,0.5f,0.5f); glVertex3f(-0.5f,0.5f,0.5f);
                    glNormal3f(0,0,-1); glVertex3f(0.5f,-0.5f,-0.5f); glVertex3f(-0.5f,-0.5f,-0.5f);
                    glVertex3f(-0.5f,0.5f,-0.5f); glVertex3f(0.5f,0.5f,-0.5f);
                glEnd();
            glPopMatrix();
        glPopMatrix();

        /* fork split right */
        glPushMatrix();
            glRotatef(ang - 9.0f, 0,0,1);
            glTranslatef(0.0f, 0.20f, s*0.065f);
            glPushMatrix();
                glScalef(0.010f, 0.055f, 0.008f);
                glBegin(GL_QUADS);
                    glNormal3f(0,0,1); glVertex3f(-0.5f,-0.5f,0.5f); glVertex3f(0.5f,-0.5f,0.5f);
                    glVertex3f(0.5f,0.5f,0.5f); glVertex3f(-0.5f,0.5f,0.5f);
                    glNormal3f(0,0,-1); glVertex3f(0.5f,-0.5f,-0.5f); glVertex3f(-0.5f,-0.5f,-0.5f);
                    glVertex3f(-0.5f,0.5f,-0.5f); glVertex3f(0.5f,0.5f,-0.5f);
                glEnd();
            glPopMatrix();
        glPopMatrix();
    }

    /* brake disc */
    setMat(0.48f,0.46f,0.42f, 0.35f, 22.0f);
    glPushMatrix();
        glTranslatef(0,0, s*0.03f);
        glRotatef(90, 1,0,0);
        renderTorus(0.16f, 0.012f, 28, 8);
    glPopMatrix();
    /* disc surface */
    glPushMatrix();
        glTranslatef(0, 0, s*0.032f);
        renderDisc(0.06f, 0.17f, 32);
    glPopMatrix();
    /* ventilation slots */
    setMat(0.08f,0.08f,0.08f, 0.05f, 3.0f);
    for (i = 0; i < 20; i++) {
        float a = i * 18.0f;
        glPushMatrix();
            glRotatef(a, 0,0,1);
            glTranslatef(0, 0.12f, s*0.035f);
            glPushMatrix();
                float sw=0.003f, sh=0.05f, sd=0.006f;
                glBegin(GL_QUADS);
                    glNormal3f(0,0,s); glVertex3f(-sw,-sh,0); glVertex3f(sw,-sh,0);
                    glVertex3f(sw,sh,0); glVertex3f(-sw,sh,0);
                glEnd();
            glPopMatrix();
        glPopMatrix();
    }

    /* red caliper */
    setMat(0.72f,0.08f,0.04f, 0.45f, 35.0f);
    glPushMatrix();
        glTranslatef(0, -0.13f, s*0.055f);
        glPushMatrix();
            glScalef(0.05f, 0.08f, 0.03f);
            glBegin(GL_QUADS);
                glNormal3f(0,0,s);
                glVertex3f(-0.5f,-0.5f, 0.5f*s); glVertex3f(0.5f,-0.5f, 0.5f*s);
                glVertex3f(0.5f, 0.5f, 0.5f*s); glVertex3f(-0.5f, 0.5f, 0.5f*s);
                glNormal3f(0,-1,0);
                glVertex3f(-0.5f,-0.5f,-0.5f); glVertex3f(0.5f,-0.5f,-0.5f);
                glVertex3f(0.5f,-0.5f, 0.5f); glVertex3f(-0.5f,-0.5f, 0.5f);
            glEnd();
        glPopMatrix();
    glPopMatrix();

    glPopMatrix();
}

/* ═══════════════════════════════════════════════
 *  KIDNEY GRILLE (vertex‑based)
 * ═══════════════════════════════════════════════ */
static void drawGrille(float cx, float cy, float cz)
{
    int i;
    /* chrome oval surround */
    setMat(0.85f,0.87f,0.90f, 0.94f, 115.0f);
    glPushMatrix();
        glTranslatef(cx, cy, cz);
        glRotatef(90, 0,1,0);
        renderTorus(0.008f, 0.055f, 20, 10);
    glPopMatrix();
    /* black fill */
    setMat(0.02f,0.02f,0.02f, 0.04f, 2.0f);
    glPushMatrix();
        glTranslatef(cx, cy, cz + 0.01f);
        renderDisc(0.0f, 0.050f, 20);
    glPopMatrix();
    /* chrome slats */
    setMat(0.82f,0.84f,0.88f, 0.90f, 108.0f);
    for (i = -2; i <= 2; i++) {
        glPushMatrix();
            glTranslatef(cx + i*0.012f, cy, cz + 0.015f);
            glBegin(GL_QUADS);
                glNormal3f(0,0,1);
                glVertex3f(-0.002f, -0.042f, 0);
                glVertex3f( 0.002f, -0.042f, 0);
                glVertex3f( 0.002f,  0.042f, 0);
                glVertex3f(-0.002f,  0.042f, 0);
            glEnd();
        glPopMatrix();
    }
}

/* ═══════════════════════════════════════════════
 *  HEADLIGHT (angel‑eye, vertex geometry)
 * ═══════════════════════════════════════════════ */
static void drawHeadlight(float cx, float cy, float cz)
{
    /* reflector bowl (chrome) */
    setMat(0.88f,0.90f,0.92f, 0.95f, 120.0f);
    glPushMatrix();
        glTranslatef(cx, cy, cz);
        /* partial sphere made from quad strips */
        int i, j, ns=16, nr=8;
        float rr = 0.08f;
        for (i = 0; i < ns; i++) {
            float a0 = (float)i/ns*2*(float)M_PI;
            float a1 = (float)(i+1)/ns*2*(float)M_PI;
            glBegin(GL_QUAD_STRIP);
            for (j = 0; j <= nr; j++) {
                float p = (float)j/nr * (float)M_PI * 0.5f;
                float cp=cosf(p), sp=sinf(p);
                glNormal3f(cosf(a0)*sp, sinf(a0)*sp, cp);
                glVertex3f(cx + rr*cosf(a0)*sp, cy + rr*sinf(a0)*sp, cz + rr*cp);
                glNormal3f(cosf(a1)*sp, sinf(a1)*sp, cp);
                glVertex3f(cx + rr*cosf(a1)*sp, cy + rr*sinf(a1)*sp, cz + rr*cp);
            }
            glEnd();
        }
    glPopMatrix();

    /* angel eye ring */
    setMat(0.85f,0.88f,1.0f, 0.95f, 100.0f);
    glPushMatrix();
        glTranslatef(cx, cy, cz + 0.02f);
        glRotatef(90, 0,1,0);
        renderTorus(0.006f, 0.058f, 24, 8);
    glPopMatrix();

    /* projector lens */
    setMat(0.95f,0.97f,1.0f, 0.98f, 128.0f);
    glPushMatrix();
        glTranslatef(cx, cy, cz + 0.06f);
        /* small sphere approximation */
        int ii, jj;
        for (ii = 0; ii < 12; ii++) {
            float la0 = (float)ii/12*2*(float)M_PI;
            float la1 = (float)(ii+1)/12*2*(float)M_PI;
            glBegin(GL_QUAD_STRIP);
            for (jj = 0; jj <= 6; jj++) {
                float lp = (float)jj/6 * (float)M_PI * 0.5f;
                float cr=0.025f;
                glNormal3f(cosf(la0)*sinf(lp), sinf(la0)*sinf(lp), cosf(lp));
                glVertex3f(cx+cr*cosf(la0)*sinf(lp), cy+cr*sinf(la0)*sinf(lp), cz+0.06f+cr*cosf(lp));
                glNormal3f(cosf(la1)*sinf(lp), sinf(la1)*sinf(lp), cosf(lp));
                glVertex3f(cx+cr*cosf(la1)*sinf(lp), cy+cr*sinf(la1)*sinf(lp), cz+0.06f+cr*cosf(lp));
            }
            glEnd();
        }
    glPopMatrix();

    /* housing surround (dark) */
    setMat(0.06f,0.06f,0.07f, 0.15f, 8.0f);
    glPushMatrix();
        glTranslatef(cx, cy, cz);
        glRotatef(90, 0,1,0);
        renderTorus(0.012f, 0.090f, 20, 8);
    glPopMatrix();
}

/* ═══════════════════════════════════════════════
 *  TAILLIGHT
 * ═══════════════════════════════════════════════ */
static void drawTaillight(float cx, float cy, float cz)
{
    /* red lens body */
    setMat(0.70f,0.03f,0.02f, 0.50f, 42.0f);
    glPushMatrix();
        glTranslatef(cx, cy, cz);
        int ii,jj;
        float tr=0.07f;
        for(ii=0;ii<12;ii++){
            float a0=(float)ii/12*2*(float)M_PI;
            float a1=(float)(ii+1)/12*2*(float)M_PI;
            glBegin(GL_QUAD_STRIP);
            for(jj=0;jj<=6;jj++){
                float p=(float)jj/6*(float)M_PI*0.5f;
                glNormal3f(cosf(a0)*sinf(p), sinf(a0)*sinf(p), -cosf(p));
                glVertex3f(cx+tr*cosf(a0)*sinf(p)*1.2f, cy+tr*sinf(a0)*sinf(p)*1.5f, cz-tr*cosf(p));
                glNormal3f(cosf(a1)*sinf(p), sinf(a1)*sinf(p), -cosf(p));
                glVertex3f(cx+tr*cosf(a1)*sinf(p)*1.2f, cy+tr*sinf(a1)*sinf(p)*1.5f, cz-tr*cosf(p));
            }
            glEnd();
        }
    glPopMatrix();

    /* amber section */
    setMat(0.90f,0.50f,0.03f, 0.45f, 35.0f);
    glPushMatrix();
        glTranslatef(cx, cy - 0.06f, cz - 0.01f);
        float ar = 0.025f;
        for(ii=0;ii<10;ii++){
            float a0=(float)ii/10*2*(float)M_PI;
            float a1=(float)(ii+1)/10*2*(float)M_PI;
            glBegin(GL_QUAD_STRIP);
            for(jj=0;jj<=5;jj++){
                float p=(float)jj/5*(float)M_PI*0.5f;
                glNormal3f(cosf(a0)*sinf(p), sinf(a0)*sinf(p), -cosf(p));
                glVertex3f(cx+ar*cosf(a0)*sinf(p), cy-0.06f+ar*sinf(a0)*sinf(p), cz-0.01f-ar*cosf(p));
                glNormal3f(cosf(a1)*sinf(p), sinf(a1)*sinf(p), -cosf(p));
                glVertex3f(cx+ar*cosf(a1)*sinf(p), cy-0.06f+ar*sinf(a1)*sinf(p), cz-0.01f-ar*cosf(p));
            }
            glEnd();
        }
    glPopMatrix();
}

/* ═══════════════════════════════════════════════
 *  GROUND PLANE (reflective showroom)
 * ═══════════════════════════════════════════════ */
static void drawGround(void)
{
    int i, j, n = 40;
    float sz = 0.45f, start = -n*sz*0.5f;
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++) {
            float br = ((i+j)%2) ? 0.18f : 0.13f;
            setMat(br,br,br+0.02f, 0.30f, 25.0f);
            float x = start + i*sz + sz*0.5f;
            float z = start + j*sz + sz*0.5f;
            glBegin(GL_QUADS);
                glNormal3f(0,1,0);
                glVertex3f(x - sz*0.5f, 0, z - sz*0.5f);
                glVertex3f(x + sz*0.5f, 0, z - sz*0.5f);
                glVertex3f(x + sz*0.5f, 0, z + sz*0.5f);
                glVertex3f(x - sz*0.5f, 0, z + sz*0.5f);
            glEnd();
        }
}

/* ═══════════════════════════════════════════════
 *  STUDIO LIGHTING
 * ═══════════════════════════════════════════════ */
static void setupLights(void)
{
    float z[4]={0,0,0,1};
    /* key — warm, upper front left */
    float p0[4]={4,7,5,1}, d0[4]={1.0f,0.96f,0.90f,1}, s0[4]={1,1,1,1};
    glLightfv(GL_LIGHT0,GL_POSITION,p0);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,d0);
    glLightfv(GL_LIGHT0,GL_SPECULAR,s0);
    glLightfv(GL_LIGHT0,GL_AMBIENT,z);
    glEnable(GL_LIGHT0);

    /* fill — cool, right */
    float p1[4]={-5,3,-3,1}, d1[4]={0.25f,0.30f,0.40f,1};
    glLightfv(GL_LIGHT1,GL_POSITION,p1);
    glLightfv(GL_LIGHT1,GL_DIFFUSE,d1);
    glLightfv(GL_LIGHT1,GL_SPECULAR,z);
    glLightfv(GL_LIGHT1,GL_AMBIENT,z);
    glEnable(GL_LIGHT1);

    /* rim — behind */
    float p2[4]={0,5,-7,1}, d2[4]={0.35f,0.38f,0.48f,1};
    glLightfv(GL_LIGHT2,GL_POSITION,p2);
    glLightfv(GL_LIGHT2,GL_DIFFUSE,d2);
    glLightfv(GL_LIGHT2,GL_SPECULAR,s0);
    glLightfv(GL_LIGHT2,GL_AMBIENT,z);
    glEnable(GL_LIGHT2);

    /* low accent — subtle ground bounce */
    float p3[4]={0,-1,2,1}, d3[4]={0.08f,0.08f,0.10f,1};
    glLightfv(GL_LIGHT3,GL_POSITION,p3);
    glLightfv(GL_LIGHT3,GL_DIFFUSE,d3);
    glLightfv(GL_LIGHT3,GL_SPECULAR,z);
    glLightfv(GL_LIGHT3,GL_AMBIENT,z);
    glEnable(GL_LIGHT3);

    float ga[4]={0.10f,0.11f,0.14f,1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ga);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
}

/* ═══════════════════════════════════════════════
 *  WINDOW GLASS (transparent quads in body)
 * ═══════════════════════════════════════════════ */
static void drawWindows(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    float gc[4] = {0.12f,0.16f,0.22f, 0.45f};
    float gs[4] = {0.85f,0.87f,0.90f, 0.45f};
    float ga[4] = {0.02f,0.03f,0.04f, 0.45f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ga);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, gc);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, gs);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

    /* windshield */
    glBegin(GL_QUADS);
        glNormal3f(0, 0.40f, 0.92f);
        glVertex3f(-0.56f, 0.92f, 0.48f);
        glVertex3f( 0.56f, 0.92f, 0.48f);
        glVertex3f( 0.48f, 1.34f, 0.18f);
        glVertex3f(-0.48f, 1.34f, 0.18f);
    glEnd();

    /* rear window */
    glBegin(GL_QUADS);
        glNormal3f(0, 0.35f, -0.94f);
        glVertex3f(-0.48f, 0.92f, -0.80f);
        glVertex3f( 0.48f, 0.92f, -0.80f);
        glVertex3f( 0.40f, 1.28f, -0.60f);
        glVertex3f(-0.40f, 1.28f, -0.60f);
    glEnd();

    /* side windows left */
    glBegin(GL_QUADS);
        glNormal3f(1, 0.08f, 0);
        glVertex3f(0.77f, 0.92f,  0.42f);
        glVertex3f(0.77f, 0.92f, -0.72f);
        glVertex3f(0.66f, 1.30f, -0.56f);
        glVertex3f(0.60f, 1.30f,  0.22f);
    glEnd();

    /* side windows right */
    glBegin(GL_QUADS);
        glNormal3f(-1, 0.08f, 0);
        glVertex3f(-0.77f, 0.92f,  0.42f);
        glVertex3f(-0.77f, 0.92f, -0.72f);
        glVertex3f(-0.66f, 1.30f, -0.56f);
        glVertex3f(-0.60f, 1.30f,  0.22f);
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

/* ═══════════════════════════════════════════════
 *  ASSEMBLE THE FULL CAR
 * ═══════════════════════════════════════════════ */
static void drawCar(void)
{
    glPushMatrix();
    glTranslatef(0, 0.35f, 0);   /* lift so tyres sit on ground */

    /* body shell */
    renderBody();

    /* windows */
    drawWindows();

    /* kidney grilles */
    drawGrille( 0.06f, 0.56f, 2.20f);
    drawGrille(-0.06f, 0.56f, 2.20f);

    /* headlights */
    drawHeadlight( 0.46f, 0.58f, 2.10f);
    drawHeadlight(-0.46f, 0.58f, 2.10f);

    /* taillights */
    drawTaillight( 0.52f, 0.56f, -2.16f);
    drawTaillight(-0.52f, 0.56f, -2.16f);

    /* exhaust tips */
    setMat(0.52f,0.54f,0.56f, 0.80f, 92.0f);
    glPushMatrix();
        glTranslatef( 0.26f, 0.14f, -2.24f);
        glRotatef(90, 1,0,0);
        renderTorus(0.012f, 0.028f, 16, 8);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(-0.26f, 0.14f, -2.24f);
        glRotatef(90, 1,0,0);
        renderTorus(0.012f, 0.028f, 16, 8);
    glPopMatrix();

    /* chrome window trim lines */
    setMat(0.84f,0.86f,0.90f, 0.92f, 110.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
        glVertex3f(-0.50f, 1.35f, 0.16f);
        glVertex3f( 0.50f, 1.35f, 0.16f);
    glEnd();
    glBegin(GL_LINE_STRIP);
        glVertex3f(0.78f, 0.92f, 0.42f);
        glVertex3f(0.68f, 1.32f, 0.20f);
        glVertex3f(0.60f, 1.36f, -0.20f);
        glVertex3f(0.66f, 1.32f, -0.56f);
        glVertex3f(0.78f, 0.92f, -0.72f);
    glEnd();
    glBegin(GL_LINE_STRIP);
        glVertex3f(-0.78f, 0.92f, 0.42f);
        glVertex3f(-0.68f, 1.32f, 0.20f);
        glVertex3f(-0.60f, 1.36f, -0.20f);
        glVertex3f(-0.66f, 1.32f, -0.56f);
        glVertex3f(-0.78f, 0.92f, -0.72f);
    glEnd();
    glLineWidth(1.0f);

    /* character lines on body sides */
    setPaint();
    /* (these are just slightly raised ridges rendered via lines for highlight) */
    setMat(paints[curPaint].r*1.08f, paints[curPaint].g*1.08f,
           paints[curPaint].b*1.08f, 0.90f, 100.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_STRIP);
        glVertex3f(0.89f, 0.58f,  1.40f);
        glVertex3f(0.90f, 0.58f,  0.00f);
        glVertex3f(0.88f, 0.58f, -1.40f);
    glEnd();
    glBegin(GL_LINE_STRIP);
        glVertex3f(-0.89f, 0.58f,  1.40f);
        glVertex3f(-0.90f, 0.58f,  0.00f);
        glVertex3f(-0.88f, 0.58f, -1.40f);
    glEnd();
    glLineWidth(1.0f);

    /* wheels */
    float wb = 2.72f, tw = 1.55f;
    glPushMatrix();
        glTranslatef( tw*0.5f, 0.0f,  wb*0.5f - 0.20f);
        drawWheel(0);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(-tw*0.5f, 0.0f,  wb*0.5f - 0.20f);
        drawWheel(1);
    glPopMatrix();
    glPushMatrix();
        glTranslatef( tw*0.5f, 0.0f, -wb*0.5f + 0.10f);
        drawWheel(0);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(-tw*0.5f, 0.0f, -wb*0.5f + 0.10f);
        drawWheel(1);
    glPopMatrix();

    glPopMatrix();
}

/* ═══════════════════════════════════════════════
 *  GLUT CALLBACKS
 * ═══════════════════════════════════════════════ */
static void reshape(int w, int h)
{
    if(h==0) h=1;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(38.0, (double)w/h, 0.1, 80.0);
    glMatrixMode(GL_MODELVIEW);
}

static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float ex = camDist * sinf(RAD(camYaw))  * cosf(RAD(camPitch));
    float ey = camDist * sinf(RAD(camPitch));
    float ez = camDist * cosf(RAD(camYaw))  * cosf(RAD(camPitch));
    gluLookAt(ex+camPanX, ey+camPanY+0.7f, ez,
              camPanX,     camPanY+0.5f,    0,
              0, 1, 0);

    setupLights();
    drawGround();

    glPushMatrix();
        glRotatef(turnAngle, 0,1,0);
        drawCar();
    glPopMatrix();

    /* HUD */
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity();
        int vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
        gluOrtho2D(0,vp[2],0,vp[3]);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
            glLoadIdentity();
            glColor3f(0.88f,0.90f,0.94f);
            glRasterPos2i(14, vp[3]-22);
            char hud[128];
            snprintf(hud,sizeof(hud),
                "BMW E46 3-Series (2005) — %s  |  SPACE=rotate  1-5=paint  Mouse=orbit  ESC=quit",
                paints[curPaint].name);
            const char *c; for(c=hud;*c;c++) glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*c);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

static void timer(int v)
{
    (void)v;
    if(autoRot){ turnAngle+=turnSpd; if(turnAngle>=360) turnAngle-=360; }
    glutPostRedisplay();
    glutTimerFunc(16,timer,0);
}

static void keyboard(unsigned char k, int x, int y)
{
    (void)x;(void)y;
    switch(k){
        case ' ': autoRot=!autoRot; break;
        case 'r':case'R': camYaw=25;camPitch=-8;camDist=7;camPanX=camPanY=0;turnAngle=0; break;
        case 'w':case'W': camDist-=0.25f; if(camDist<2.5f)camDist=2.5f; break;
        case 's':case'S': camDist+=0.25f; if(camDist>18)camDist=18; break;
        case 'a':case'A': camPanX-=0.10f; break;
        case 'd':case'D': camPanX+=0.10f; break;
        case'+':case'=': turnSpd+=0.12f; break;
        case'-':case'_': turnSpd-=0.12f; if(turnSpd<0.02f)turnSpd=0.02f; break;
        case'1': curPaint=0; break;
        case'2': curPaint=1; break;
        case'3': curPaint=2; break;
        case'4': curPaint=3; break;
        case'5': curPaint=4; break;
        case 27: exit(0);
    }
}

static void specialKey(int k, int x, int y)
{
    (void)x;(void)y;
    switch(k){
        case GLUT_KEY_LEFT:  camYaw-=4; break;
        case GLUT_KEY_RIGHT: camYaw+=4; break;
        case GLUT_KEY_UP:    camPitch+=3; if(camPitch>89)camPitch=89; break;
        case GLUT_KEY_DOWN:  camPitch-=3; if(camPitch<-89)camPitch=-89; break;
    }
}

static void mouseBtn(int b, int s, int x, int y)
{
    if(b==GLUT_LEFT_BUTTON){ mouseDown=(s==GLUT_DOWN); lastMX=x;lastMY=y; }
    if(b==3){ camDist-=0.3f; if(camDist<2.5f)camDist=2.5f; }
    if(b==4){ camDist+=0.3f; if(camDist>18)camDist=18; }
}

static void mouseMove(int x, int y)
{
    if(!mouseDown) return;
    camYaw+=(x-lastMX)*0.5f;
    camPitch+=(y-lastMY)*0.5f;
    if(camPitch>89)camPitch=89;
    if(camPitch<-89)camPitch=-89;
    lastMX=x;lastMY=y;
}

/* ═══════════════════════════════════════════════
 *  MAIN
 * ═══════════════════════════════════════════════ */
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 750);
    glutInitWindowPosition(60, 40);
    glutCreateWindow("BMW E46 3-Series (2005) — Raw OpenGL Mesh");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    /* atmospheric fog */
    glEnable(GL_FOG);
    float fc[4]={0.07f,0.08f,0.12f,1};
    glFogfv(GL_FOG_COLOR,fc);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 10.0f);
    glFogf(GL_FOG_END,   28.0f);

    /* init procedural data */
    generateEnvMap();
    initBodyMesh();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKey);
    glutMouseFunc(mouseBtn);
    glutMotionFunc(mouseMove);
    glutTimerFunc(16,timer,0);

    printf("============================================\n");
    printf("  BMW E46 3-Series (2005)\n");
    printf("  Raw OpenGL Mesh + Environment Reflections\n");
    printf("============================================\n");
    printf("  SPACE       turntable on/off\n");
    printf("  1-5         paint colour\n");
    printf("  W/S         zoom       A/D  pan\n");
    printf("  Arrows      pitch / yaw\n");
    printf("  Mouse drag  orbit      Scroll zoom\n");
    printf("  +/-         turntable speed\n");
    printf("  R           reset      ESC  quit\n");
    printf("============================================\n");

    glutMainLoop();
    return 0;
}
