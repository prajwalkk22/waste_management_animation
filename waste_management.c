/*
 ╔══════════════════════════════════════════════════════════════════════╗
 ║     URBAN WASTE MANAGEMENT — A 5-MINUTE OPENGL/GLUT ANIMATION       ║
 ║     Modern Smart City Approach to Zero-Waste Urban Living            ║
 ╠══════════════════════════════════════════════════════════════════════╣
 ║  COMPILE:                                                            ║
 ║    Linux:   gcc waste_management.c -o waste -lGL -lGLU -lglut -lm   ║
 ║    macOS:   gcc waste_management.c -o waste \                        ║
 ║               -framework OpenGL -framework GLUT -lm                  ║
 ║    Windows: gcc waste_management.c -o waste \                        ║
 ║               -lglut32 -lglu32 -lopengl32 -lm                       ║
 ╠══════════════════════════════════════════════════════════════════════╣
 ║  SCENES (300 seconds total):                                         ║
 ║    S0  0:00–0:12  Cinematic Title                                    ║
 ║    S1  0:12–0:55  The Problem: Overflowing City Waste                ║
 ║    S2  0:55–1:45  Smart Bin Network & IoT Sensors                   ║
 ║    S3  1:45–2:35  Automated Collection Trucks & Routes               ║
 ║    S4  2:35–3:20  Sorting Facility & Recycling Process               ║
 ║    S5  3:20–4:05  Waste-to-Energy Plant                              ║
 ║    S6  4:05–4:35  City Dashboard & Analytics                         ║
 ║    S7  4:35–5:00  Zero-Waste Future Vision                           ║
 ╠══════════════════════════════════════════════════════════════════════╣
 ║  CONTROLS:                                                           ║
 ║    [P]      Pause / Resume                                           ║
 ║    [R]      Restart                                                  ║
 ║    [1-8]    Jump to scene                                            ║
 ║    [Q/Esc]  Quit                                                     ║
 ╚══════════════════════════════════════════════════════════════════════╝
*/

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ══════════════════════════════════════════════════════
   CONFIGURATION
   ══════════════════════════════════════════════════════ */
#define TRUCK_SPEED_BASE     50.0f
#define BIN_ALERT_THRESHOLD  0.75f
#define PARTICLE_GRAVITY     20.0f
#define FLAME_LAYERS         6
#define GLOW_LAYERS          6
#define TRANSITION_DURATION  1.2f
#define VIGNETTE_STRENGTH    0.45f
#define MOTION_BLUR_ALPHA    0.12f
#define SECONDARY_MOTION_AMP 2.0f

/* ══════════════════════════════════════════════════════
   CONSTANTS & GLOBALS
   ══════════════════════════════════════════════════════ */
#define W  1280
#define H  720
#define PI 3.14159265358979f
#define TAU (2.0f * PI)

/* Scene boundaries (seconds) */
#define T0  0.0f
#define T1  12.0f
#define T2  55.0f
#define T3  105.0f
#define T4  155.0f
#define T5  200.0f
#define T6  245.0f
#define T7  275.0f
#define T8  300.0f

static float  gt    = 0.0f;   /* global time */
static int    scene = 0;
static int    paused= 0;
static int    lastMs= 0;
static float  fps   = 60.0f;

/* Transition system */
static int    prev_scene = -1;
static float  trans_timer = 0.0f;
static int    in_transition = 0;

/* ══════════════════════════════════════════════════════
   MATH HELPERS
   ══════════════════════════════════════════════════════ */
static float clamp01(float v){ return v<0?0:v>1?1:v; }
static float lerpf(float a,float b,float t){ return a+(b-a)*clamp01(t); }
static float smoothstep(float t){ t=clamp01(t); return t*t*(3-2*t); }
static float ease_out(float t){ t=clamp01(t); return 1-(1-t)*(1-t); }
static float ease_in(float t){ t=clamp01(t); return t*t; }
static float ease_in_out(float t){ t=clamp01(t); return t<0.5f?2*t*t:1-powf(-2*t+2,2)/2; }
static float bounce_out(float t){ t=clamp01(t); if(t<1/2.75f) return 7.5625f*t*t; else if(t<2/2.75f){t-=1.5f/2.75f;return 7.5625f*t*t+0.75f;} else if(t<2.5f/2.75f){t-=2.25f/2.75f;return 7.5625f*t*t+0.9375f;} else{t-=2.625f/2.75f;return 7.5625f*t*t+0.984375f;} }
static float pulse(float freq){ return 0.5f+0.5f*sinf(gt*freq); }
static float wave(float freq,float phase){ return sinf(gt*freq+phase); }

/* Bezier interpolation for curved paths */
static float bezier3(float p0,float p1,float p2,float p3,float t){
    float u=1-t;
    return u*u*u*p0+3*u*u*t*p1+3*u*t*t*p2+t*t*t*p3;
}

/* Local scene time [0..1] */
static float st(float s, float e){ return clamp01((gt-s)/(e-s)); }
/* Local scene seconds */
static float ss(float s){ return gt-s; }

/* ══════════════════════════════════════════════════════
   DRAW PRIMITIVES
   ══════════════════════════════════════════════════════ */
static void Col3(float r,float g,float b){ glColor3f(r,g,b); }
static void Col4(float r,float g,float b,float a){ glColor4f(r,g,b,a); }

static void Rect(float x,float y,float w,float h){
    glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}

static void RectGradV(float x,float y,float w,float h,
                       float r1,float g1,float b1,float a1,
                       float r2,float g2,float b2,float a2){
    glBegin(GL_QUADS);
    glColor4f(r1,g1,b1,a1); glVertex2f(x,y);
    glColor4f(r1,g1,b1,a1); glVertex2f(x+w,y);
    glColor4f(r2,g2,b2,a2); glVertex2f(x+w,y+h);
    glColor4f(r2,g2,b2,a2); glVertex2f(x,y+h);
    glEnd();
}

static void RectGradH(float x,float y,float w,float h,
                       float r1,float g1,float b1,
                       float r2,float g2,float b2){
    glBegin(GL_QUADS);
    glColor3f(r1,g1,b1); glVertex2f(x,y);
    glColor3f(r2,g2,b2); glVertex2f(x+w,y);
    glColor3f(r2,g2,b2); glVertex2f(x+w,y+h);
    glColor3f(r1,g1,b1); glVertex2f(x,y+h);
    glEnd();
}

static void Circle(float cx,float cy,float r,int seg){
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx,cy);
    for(int i=0;i<=seg;i++){
        float a=TAU*i/seg;
        glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);
    }
    glEnd();
}

static void CircleOutline(float cx,float cy,float r,int seg,float lw){
    glLineWidth(lw);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<seg;i++){
        float a=TAU*i/seg;
        glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);
    }
    glEnd();
}

static void Ring(float cx,float cy,float r,float t2,int seg){
    glBegin(GL_QUAD_STRIP);
    for(int i=0;i<=seg;i++){
        float a=TAU*i/seg;
        float c=cosf(a),s=sinf(a);
        glVertex2f(cx+c*(r-t2),cy+s*(r-t2));
        glVertex2f(cx+c*r,     cy+s*r);
    }
    glEnd();
}

static void Line2(float x1,float y1,float x2,float y2){
    glBegin(GL_LINES);
    glVertex2f(x1,y1); glVertex2f(x2,y2);
    glEnd();
}

static void Tri(float x1,float y1,float x2,float y2,float x3,float y3){
    glBegin(GL_TRIANGLES);
    glVertex2f(x1,y1); glVertex2f(x2,y2); glVertex2f(x3,y3);
    glEnd();
}

/* Dashed line with animation */
static void DashLine(float x1,float y1,float x2,float y2,float dashlen,float gap,float phase){
    float dx=x2-x1,dy=y2-y1;
    float len=sqrtf(dx*dx+dy*dy);
    if(len<0.001f) return;
    dx/=len; dy/=len;
    float period=dashlen+gap;
    float start=fmodf(-phase*period,period);
    if(start>0) start-=period;
    for(float s=start;s<len;s+=period){
        float e=s+dashlen;
        float a=s<0?0:s, b=e>len?len:e;
        if(b>a && a<len && b>0){
            glBegin(GL_LINES);
            glVertex2f(x1+dx*a,y1+dy*a);
            glVertex2f(x1+dx*b,y1+dy*b);
            glEnd();
        }
    }
}

/* Text helpers */
static void Text(float x,float y,const char*s,void*font){
    glRasterPos2f(x,y);
    for(const char*c=s;*c;c++) glutBitmapCharacter(font,*c);
}

static void TextC(float cx,float y,const char*s,void*font){
    int w2=glutBitmapLength(font,(const unsigned char*)s)/2;
    glRasterPos2f(cx-w2,y);
    for(const char*c=s;*c;c++) glutBitmapCharacter(font,*c);
}

/* ══════════════════════════════════════════════════════
   BLEND HELPERS
   ══════════════════════════════════════════════════════ */
static void BlendOn(){ glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); }
static void BlendAdd(){ glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE); }
static void BlendOff(){ glDisable(GL_BLEND); }

/* Fade alpha based on scene progress */
static float fade_alpha(float s, float e){
    float t=st(s,e);
    return t<0.08f ? t/0.08f : t>0.92f ? (1-t)/0.08f : 1.0f;
}

/* ══════════════════════════════════════════════════════
   ADVANCED VISUAL EFFECTS
   ══════════════════════════════════════════════════════ */

/* Multi-layered radial glow (additive blend) */
static void drawGlow(float x,float y,float r,float cr,float cg,float cb){
    BlendAdd();
    for(int i=GLOW_LAYERS;i>=1;i--){
        float rr=r*i/(float)GLOW_LAYERS;
        float a=0.04f*(GLOW_LAYERS+1-i);
        Col4(cr,cg,cb,a);
        Circle(x,y,rr,24);
    }
    BlendOff();
}

/* Soft elliptical shadow beneath objects */
static void drawShadow(float x,float y,float w,float h){
    BlendOn();
    Col4(0,0,0,0.22f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x,y);
    for(int i=0;i<=24;i++){
        float a=TAU*i/24.0f;
        glVertex2f(x+cosf(a)*w,y+sinf(a)*h*0.3f);
    }
    glEnd();
    BlendOff();
}

/* Triangular light cone (for streetlamps, headlights) */
static void drawLightCone(float x,float y,float angle,float spread,float dist,
                           float cr,float cg,float cb,float alpha){
    BlendAdd();
    float a1=(angle-spread/2)*PI/180.0f;
    float a2=(angle+spread/2)*PI/180.0f;
    /* Multi-layer fade */
    for(int i=3;i>=1;i--){
        float d=dist*i/3.0f;
        float aa=alpha*0.12f*(4-i);
        Col4(cr,cg,cb,aa);
        glBegin(GL_TRIANGLES);
        glVertex2f(x,y);
        glVertex2f(x+cosf(a1)*d,y+sinf(a1)*d);
        glVertex2f(x+cosf(a2)*d,y+sinf(a2)*d);
        glEnd();
    }
    BlendOff();
}

/* Cinematic vignette - dark edges */
static void drawVignette(void){
    BlendOn();
    /* Top */
    RectGradV(0,H-130,W,130, 0,0,0,0, 0,0,0,VIGNETTE_STRENGTH);
    /* Bottom */
    RectGradV(0,0,W,100, 0,0,0,VIGNETTE_STRENGTH, 0,0,0,0);
    /* Left */
    Col4(0,0,0,VIGNETTE_STRENGTH*0.6f);
    glBegin(GL_QUADS);
    glColor4f(0,0,0,VIGNETTE_STRENGTH*0.6f); glVertex2f(0,0);
    glColor4f(0,0,0,0);                      glVertex2f(140,0);
    glColor4f(0,0,0,0);                      glVertex2f(140,H);
    glColor4f(0,0,0,VIGNETTE_STRENGTH*0.6f); glVertex2f(0,H);
    glEnd();
    /* Right */
    glBegin(GL_QUADS);
    glColor4f(0,0,0,0);                      glVertex2f(W-140,0);
    glColor4f(0,0,0,VIGNETTE_STRENGTH*0.6f); glVertex2f(W,0);
    glColor4f(0,0,0,VIGNETTE_STRENGTH*0.6f); glVertex2f(W,H);
    glColor4f(0,0,0,0);                      glVertex2f(W-140,H);
    glEnd();
    BlendOff();
}

/* Full-screen fade overlay for transitions */
static void drawScreenFade(float alpha){
    if(alpha<0.005f) return;
    BlendOn();
    Col4(0,0,0,alpha);
    Rect(0,0,W,H);
    BlendOff();
}

/* Typewriter text - shows chars up to a reveal count */
static void TextPartial(float x,float y,const char*s,void*font,int chars_shown){
    glRasterPos2f(x,y);
    int i=0;
    for(const char*c=s;*c && i<chars_shown;c++,i++)
        glutBitmapCharacter(font,*c);
}

/* Centered typewriter text */
static void TextCPartial(float cx,float y,const char*s,void*font,int chars_shown){
    int w2=glutBitmapLength(font,(const unsigned char*)s)/2;
    glRasterPos2f(cx-w2,y);
    int i=0;
    for(const char*c=s;*c && i<chars_shown;c++,i++)
        glutBitmapCharacter(font,*c);
}


/* ══════════════════════════════════════════════════════
   SHARED VISUAL: Enhanced Particle System
   ══════════════════════════════════════════════════════ */
#define MAX_PARTICLES 600
/* Particle types: 0=default, 1=smoke, 2=spark, 3=leaf, 4=energy, 5=dust, 6=ember */
typedef struct{
    float x,y,vx,vy,life,maxlife,r,g,b,size;
    int   type;    /* rendering style */
    float drag;    /* velocity decay per second (0=none, 1=full stop) */
    float grav;    /* gravity multiplier (1=normal, 0=none, -1=float up) */
} Particle;
static Particle parts[MAX_PARTICLES];
static int nparts=0;

/* Full-featured spawn */
static void SpawnParticleEx(float x,float y,float vx,float vy,float life,
                             float r,float g,float b,float sz,
                             int type,float drag,float grav){
    if(nparts>=MAX_PARTICLES) return;
    Particle*p=&parts[nparts++];
    p->x=x; p->y=y; p->vx=vx; p->vy=vy;
    p->life=life; p->maxlife=life;
    p->r=r; p->g=g; p->b=b; p->size=sz;
    p->type=type; p->drag=drag; p->grav=grav;
}

/* Legacy-compatible spawn (type=0, default gravity, no drag) */
static void SpawnParticle(float x,float y,float vx,float vy,float life,
                           float r,float g,float b,float sz){
    SpawnParticleEx(x,y,vx,vy,life,r,g,b,sz, 0,0.0f,1.0f);
}

static void UpdateParticles(float dt){
    for(int i=0;i<nparts;){
        Particle*p=&parts[i];
        p->life-=dt;
        if(p->life<=0){ parts[i]=parts[--nparts]; continue; }
        /* Apply drag */
        float d=1.0f-p->drag*dt*3.0f;
        if(d<0) d=0;
        p->vx*=d; p->vy*=d;
        /* Move */
        p->x+=p->vx*dt; p->y+=p->vy*dt;
        /* Per-particle gravity */
        p->vy-=PARTICLE_GRAVITY*p->grav*dt;
        i++;
    }
}

static void DrawParticles(void){
    BlendAdd();
    for(int i=0;i<nparts;i++){
        Particle*p=&parts[i];
        float a=p->life/p->maxlife;
        switch(p->type){
            case 1: /* smoke — large, soft, alpha blend */
                BlendOn();
                Col4(p->r,p->g,p->b,a*0.25f);
                Circle(p->x,p->y,p->size*(2.0f-a),16);
                BlendAdd();
                break;
            case 2: /* spark — bright dot with short trail */
                Col4(p->r,p->g,p->b,a);
                Circle(p->x,p->y,p->size*a,6);
                Col4(p->r,p->g,p->b,a*0.4f);
                Circle(p->x-p->vx*0.03f,p->y-p->vy*0.03f,p->size*a*0.7f,6);
                break;
            case 3: /* leaf — tiny triangle, wobble */
                Col4(p->r,p->g,p->b,a*0.7f);
                {float wo=sinf(gt*4+p->x*0.1f)*4;
                Tri(p->x+wo,p->y+p->size*a,
                    p->x-p->size*a*0.6f+wo,p->y-p->size*a*0.5f,
                    p->x+p->size*a*0.6f+wo,p->y-p->size*a*0.5f);}
                break;
            case 4: /* energy — bright glow */
                Col4(p->r,p->g,p->b,a*0.9f);
                Circle(p->x,p->y,p->size*a,8);
                Col4(p->r,p->g,p->b,a*0.3f);
                Circle(p->x,p->y,p->size*a*2.5f,12);
                break;
            case 5: /* dust — tiny, faint */
                Col4(p->r,p->g,p->b,a*0.35f);
                Circle(p->x,p->y,p->size*0.5f,6);
                break;
            case 6: /* ember — bright upward, flicker */
                {float flk=0.5f+0.5f*sinf(gt*12+p->y);
                Col4(p->r*flk+0.3f,p->g*flk,p->b,a*0.8f);
                Circle(p->x,p->y,p->size*a,6);}
                break;
            default: /* original style */
                Col4(p->r,p->g,p->b,a*0.8f);
                Circle(p->x,p->y,p->size*a,8);
                break;
        }
    }
    BlendOff();
}

/* Random seed per scene */
static unsigned int rng=12345;
static float frand(){ rng=rng*1664525+1013904223; return (rng&0xFFFF)/65535.0f; }

/* ══════════════════════════════════════════════════════
   SCENE ELEMENT HELPERS
   ══════════════════════════════════════════════════════ */

/* Walking person silhouette */
static void draw_person(float x,float y,float walk_phase,float dir,float cr,float cg,float cb){
    glLineWidth(2.0f);
    Col3(cr,cg,cb);
    /* Head */
    Circle(x,y+38,5,10);
    /* Body */
    Rect(x-3,y+10,6,26);
    /* Legs - animated */
    float leg=sinf(walk_phase)*8*dir;
    glBegin(GL_LINES);
    glVertex2f(x,y+12); glVertex2f(x-leg,y);
    glVertex2f(x,y+12); glVertex2f(x+leg,y);
    /* Arms */
    glVertex2f(x,y+28); glVertex2f(x-6-leg*0.5f,y+18);
    glVertex2f(x,y+28); glVertex2f(x+6+leg*0.5f,y+18);
    glEnd();
}

/* Scurrying rat */
static void draw_rat(float x,float y,float phase){
    Col3(0.18f,0.15f,0.12f);
    /* Body */
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x,y+3);
    for(int i=0;i<=12;i++){
        float a=PI*i/12.0f;
        glVertex2f(x+cosf(a)*10,y+3+sinf(a)*5);
    }
    glEnd();
    /* Head */
    Circle(x+8*cosf(phase*0.1f),y+4,4,8);
    /* Tail */
    glLineWidth(1.5f);
    Col3(0.25f,0.20f,0.15f);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<=6;i++){
        float tt=(float)i/6;
        glVertex2f(x-10-tt*12,y+3+sinf(phase+tt*4)*3);
    }
    glEnd();
    /* Legs (tiny) */
    float lp=sinf(phase*3)*2;
    Col3(0.15f,0.12f,0.10f);
    Line2(x-4,y+1,x-5+lp,y);
    Line2(x+4,y+1,x+5-lp,y);
}

/* Streetlight with glow */
static void draw_streetlight(float x,float y,float h,int on,float fa){
    /* Pole */
    Col3(0.3f,0.28f,0.25f);
    Rect(x-3,y,6,h);
    /* Arm */
    Rect(x-2,y+h-3,20,6);
    /* Lamp housing */
    Col3(0.25f,0.23f,0.20f);
    Rect(x+14,y+h-10,12,10);
    /* Light */
    if(on){
        float flicker=0.85f+0.15f*sinf(gt*7+x*0.5f);
        drawGlow(x+20,y+h-12,80*flicker, 1.0f,0.85f,0.4f);
        drawLightCone(x+20,y+h-14, 270,55,120, 1.0f,0.85f,0.4f, flicker*fa);
    }
}

/* Traffic light */
static void draw_traffic_light(float x,float y,float fa){
    /* Housing */
    Col3(0.2f,0.2f,0.22f);
    Rect(x-8,y,16,42);
    /* Pole */
    Col3(0.25f,0.25f,0.28f);
    Rect(x-3,y-20,6,20);
    /* Determine state: cycle every 8 seconds */
    int cycle=(int)(gt/3)%3;
    float glow=0.5f+0.5f*pulse(2.0f);
    /* Red */
    Col3(cycle==0?0.9f*glow:0.15f, 0.05f, 0.05f);
    Circle(x,y+34,5,10);
    if(cycle==0) drawGlow(x,y+34,18,1.0f,0.1f,0.05f);
    /* Yellow */
    Col3(cycle==1?0.9f*glow:0.15f, cycle==1?0.7f*glow:0.12f, 0.05f);
    Circle(x,y+22,5,10);
    if(cycle==1) drawGlow(x,y+22,15,1.0f,0.8f,0.1f);
    /* Green */
    Col3(0.05f, cycle==2?0.8f*glow:0.12f, 0.05f);
    Circle(x,y+10,5,10);
    if(cycle==2) drawGlow(x,y+10,18,0.1f,1.0f,0.2f);
}

/* Civilian car */
static void draw_civilian_car(float x,float y,float dir,float cr,float cg,float cb){
    glPushMatrix();
    glTranslatef(x,y,0);
    if(dir<0) glScalef(-1,1,1);
    /* Body */
    Col3(cr,cg,cb);
    glBegin(GL_QUADS);
    glVertex2f(-20,0); glVertex2f(20,0);
    glVertex2f(18,10); glVertex2f(-18,10);
    glEnd();
    /* Cabin */
    Col3(0.55f,0.78f,0.95f);
    Rect(-10,10,20,8);
    /* Wheels */
    Col3(0.12f,0.12f,0.12f);
    Circle(-12,-2,5,8);
    Circle(12,-2,5,8);
    /* Headlight */
    BlendAdd();
    Col4(1.0f,1.0f,0.6f,0.6f);
    Circle(20,5,3,6);
    /* Taillight */
    Col4(1.0f,0.1f,0.05f,0.5f);
    Circle(-20,5,3,6);
    BlendOff();
    glPopMatrix();
}

/* Bird in flight */
static void draw_bird(float x,float y,float wing_phase){
    glLineWidth(2.0f);
    Col3(0.15f,0.12f,0.10f);
    float w=sinf(wing_phase)*8;
    glBegin(GL_LINE_STRIP);
    glVertex2f(x-12,y-w*0.5f);
    glVertex2f(x-4,y+w);
    glVertex2f(x,y);
    glVertex2f(x+4,y+w);
    glVertex2f(x+12,y-w*0.5f);
    glEnd();
}

/* Wind turbine */
static void draw_wind_turbine(float x,float y,float h,float blade_angle){
    /* Tower */
    Col3(0.75f,0.75f,0.78f);
    glBegin(GL_QUADS);
    glVertex2f(x-6,y); glVertex2f(x+6,y);
    glVertex2f(x+3,y+h); glVertex2f(x-3,y+h);
    glEnd();
    /* Hub */
    Col3(0.8f,0.8f,0.82f);
    Circle(x,y+h,8,12);
    /* Blades */
    Col3(0.90f,0.90f,0.92f);
    for(int b=0;b<3;b++){
        float ba=blade_angle+TAU*b/3.0f;
        float bx=cosf(ba)*55, by=sinf(ba)*55;
        glLineWidth(4.0f);
        glBegin(GL_LINES);
        glVertex2f(x,y+h);
        glVertex2f(x+bx,y+h+by);
        glEnd();
        /* Blade shape */
        glLineWidth(1.5f);
        float perp_a=ba+PI/2;
        float pw=4;
        glBegin(GL_QUADS);
        glVertex2f(x,y+h);
        glVertex2f(x+cosf(perp_a)*pw,y+h+sinf(perp_a)*pw);
        glVertex2f(x+bx+cosf(perp_a)*pw*0.3f,y+h+by+sinf(perp_a)*pw*0.3f);
        glVertex2f(x+bx,y+h+by);
        glEnd();
    }
}

/* Rainbow arc */
static void draw_rainbow(float cx,float cy,float radius,float alpha){
    float colors[][3]={
        {1.0f,0.2f,0.2f},{1.0f,0.5f,0.1f},{1.0f,0.9f,0.2f},
        {0.2f,0.9f,0.3f},{0.2f,0.5f,1.0f},{0.3f,0.2f,0.8f},{0.5f,0.2f,0.7f}
    };
    BlendOn();
    for(int c=0;c<7;c++){
        float r=radius-c*8;
        Col4(colors[c][0],colors[c][1],colors[c][2],alpha*0.35f);
        glBegin(GL_QUAD_STRIP);
        for(int i=0;i<=30;i++){
            float a=PI*0.15f+PI*0.7f*i/30.0f;
            glVertex2f(cx+cosf(a)*(r-6),cy+sinf(a)*(r-6));
            glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);
        }
        glEnd();
    }
    BlendOff();
}

/* ══════════════════════════════════════════════════════
   SCENE 0 — CINEMATIC TITLE  (0–12s)
   ══════════════════════════════════════════════════════ */
static void draw_title(void){
    float t=st(T0,T1);
    float fa=fade_alpha(T0,T1);

    /* Breathing deep space background */
    float breath=0.02f+0.008f*sinf(gt*0.8f);
    RectGradV(0,0,W,H, 0.02f+breath,0.04f+breath,0.06f+breath*2,1,
              0.00f,0.02f,0.04f,1);

    /* Rotating hexagonal grid with color wave */
    BlendOn();
    for(int gx=-2;gx<=18;gx++){
        for(int gy=-2;gy<=12;gy++){
            float cx=gx*75.0f+35*(gy&1);
            float cy=gy*65.0f;
            float dist=sqrtf((cx-W/2.0f)*(cx-W/2.0f)+(cy-H/2.0f)*(cy-H/2.0f));
            float brightness=0.05f+0.06f*sinf(dist*0.01f-gt*0.8f);
            float hue_shift=sinf(dist*0.005f+gt*0.3f)*0.3f;
            Col4(0.1f+hue_shift*0.1f,0.6f,0.3f+hue_shift*0.2f,brightness*fa);
            CircleOutline(cx,cy,30,6,1.0f);
        }
    }
    BlendOff();

    /* Structured orbital rings */
    BlendOn();
    for(int ring=0;ring<3;ring++){
        float rr=120+ring*40;
        float speed=0.3f-ring*0.08f;
        Col4(0.15f,0.8f,0.4f,0.12f*fa);
        glLineWidth(1.0f);
        CircleOutline(W/2.0f,H/2.0f+30,rr,48,1.0f);
        /* Orbiting dots */
        for(int d=0;d<4+ring*2;d++){
            float a=gt*speed+TAU*d/(4+ring*2);
            float ox=W/2.0f+cosf(a)*rr;
            float oy=H/2.0f+30+sinf(a)*rr;
            Col4(0.3f,1.0f,0.5f,0.6f*fa);
            Circle(ox,oy,3+ring,8);
            Col4(0.3f,1.0f,0.5f,0.15f*fa);
            Circle(ox,oy,8+ring*2,12);
        }
    }
    BlendOff();

    /* Central glowing orb - enhanced with drawGlow */
    float orb_r=80+15*pulse(1.2f);
    drawGlow(W/2.0f,H/2.0f+30,orb_r*1.2f, 0.1f,0.9f,0.4f);
    BlendAdd();
    for(int ring=5;ring>=1;ring--){
        float rr=orb_r*ring/5.0f;
        float aa=0.06f*(6-ring)*fa;
        Col4(0.1f,0.9f,0.4f,aa);
        Circle(W/2.0f,H/2.0f+30,rr,48);
    }
    BlendOff();

    /* Earth/leaf icon - stylized */
    Col3(0.08f,0.5f,0.2f);
    Circle(W/2.0f,H/2.0f+30,48,48);
    Col3(0.05f,0.35f,0.15f);
    Circle(W/2.0f-15,H/2.0f+45,16,20);
    Circle(W/2.0f+12,H/2.0f+20,12,20);
    Circle(W/2.0f+5,H/2.0f+50,10,20);

    /* Recycling arrows around orb */
    glLineWidth(3.0f);
    for(int i=0;i<3;i++){
        float base=gt*0.5f+TAU*i/3.0f;
        BlendOn();
        Col4(0.3f,1.0f,0.5f,0.85f*fa);
        glBegin(GL_LINE_STRIP);
        for(int j=0;j<=20;j++){
            float a=base+TAU/3.0f*j/20.0f;
            glVertex2f(W/2.0f+cosf(a)*70,H/2.0f+30+sinf(a)*70);
        }
        glEnd();
        float ae=base+TAU/3.0f;
        float ax=W/2.0f+cosf(ae)*70, ay=H/2.0f+30+sinf(ae)*70;
        float at2=ae+PI/2.0f;
        Col4(0.3f,1.0f,0.5f,fa);
        Tri(ax+cosf(at2)*10,ay+sinf(at2)*10,
            ax-cosf(at2)*10,ay-sinf(at2)*10,
            ax+cosf(ae)*14,ay+sinf(ae)*14);
        BlendOff();
    }

    /* Title — typewriter reveal */
    BlendOn();
    int title_chars=(int)(t*12*22);  /* reveal over first ~40% of scene */
    int sub_chars=(int)((t-0.35f)*10*50);
    if(sub_chars<0) sub_chars=0;
    int anim_chars=(int)((t-0.6f)*8*20);
    if(anim_chars<0) anim_chars=0;

    /* Glow behind title text */
    if(title_chars>10){
        drawGlow(W/2.0f,H/2.0f-50,140, 0.05f,0.3f,0.1f);
    }

    Col4(0.2f,1.0f,0.5f,fa);
    TextCPartial(W/2.0f,H/2.0f-55,"URBAN WASTE MANAGEMENT",GLUT_BITMAP_TIMES_ROMAN_24,title_chars);

    Col4(1.0f,1.0f,1.0f,fa*0.85f);
    TextCPartial(W/2.0f,H/2.0f-85,"SMART CITIES  ·  ZERO WASTE  ·  SUSTAINABLE FUTURE",GLUT_BITMAP_HELVETICA_12,sub_chars);

    Col4(0.5f,1.0f,0.6f,fa*0.6f*(0.5f+0.5f*sinf(gt*3)));
    TextCPartial(W/2.0f,H/2.0f-115,"A 5-MINUTE ANIMATION",GLUT_BITMAP_HELVETICA_12,anim_chars);

    /* Enhanced sparkle particles — spark type, zero gravity */
    if(frand()<0.5f){
        float a=frand()*TAU;
        float r=60+frand()*40;
        SpawnParticleEx(W/2.0f+cosf(a)*r,H/2.0f+30+sinf(a)*r,
                        cosf(a)*20,sinf(a)*20,1.8f,
                        0.3f,1.0f,0.5f,4, 2,0.3f,0.0f);
    }
    /* Dust motes in background */
    if(frand()<0.3f){
        SpawnParticleEx(frand()*W,frand()*H,
                        (frand()-0.5f)*5,frand()*5,3.0f,
                        0.2f,0.5f,0.3f,2, 5,0.1f,0.0f);
    }
    DrawParticles();
    BlendOff();
}


/* ══════════════════════════════════════════════════════
   SCENE 1 — THE PROBLEM: OVERFLOWING CITY WASTE (12–55s)
   ══════════════════════════════════════════════════════ */
static void draw_garbage_pile(float cx,float by,float size,float fill){
    /* Pile of garbage - amorphous shape */
    srand((int)(cx*100));
    int nlayers=5+(int)(fill*8);
    for(int i=nlayers;i>=0;i--){
        float layer=(float)i/nlayers;
        float rr=lerpf(0.5f,0.85f,layer);
        float gg=lerpf(0.4f,0.65f,layer);
        float bb=lerpf(0.2f,0.35f,layer);
        Col3(rr,gg,bb);
        float w2=size*(1.0f-layer*0.4f)*(0.8f+0.2f*sinf(cx*0.1f+i*0.7f));
        float hh=size*fill*(1.0f-layer*0.15f)*0.5f;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx,by+i*hh*0.3f);
        int segs=16;
        for(int s=0;s<=segs;s++){
            float a=PI*s/segs;
            float noise=1.0f+0.15f*sinf(a*5+i*1.3f+cx*0.03f);
            glVertex2f(cx+cosf(a)*w2*noise,(by+i*hh*0.3f)+sinf(a)*hh*noise*0.5f);
        }
        glEnd();
    }
    /* Flies (small dots) */
    BlendOn();
    for(int f=0;f<6;f++){
        float fa2=gt*1.5f+f*1.1f;
        float fx=cx+cosf(fa2)*size*0.8f*(0.5f+0.3f*sinf(fa2*1.7f));
        float fy=by+size*fill*0.4f+sinf(fa2*0.8f)*20;
        Col4(0.1f,0.1f,0.1f,0.7f);
        Circle(fx,fy,2.5f,6);
    }
    BlendOff();
    /* Stink lines */
    BlendOn();
    for(int s=0;s<3;s++){
        float sp=gt*20+s*30;
        float sy=fmodf(sp,60);
        float sx=cx+(-15+s*15)+sinf(gt*2+s)*8;
        float sa=0.4f*(1-sy/60.0f);
        Col4(0.6f,0.7f,0.2f,sa);
        glLineWidth(1.5f);
        glBegin(GL_LINE_STRIP);
        for(int p=0;p<=8;p++){
            float py=(float)p/8*sy;
            glVertex2f(sx+sinf(p*0.8f)*6,by+size*fill*0.5f+py);
        }
        glEnd();
    }
    BlendOff();
}

static void draw_problem_city(void){
    float t=st(T1,T2);
    float fa=fade_alpha(T1,T2);

    /* Smoggy sky gradient */
    float smog=0.3f+0.3f*t;
    RectGradV(0,0,W,H,
              lerpf(0.55f,0.35f,smog),lerpf(0.48f,0.32f,smog),lerpf(0.35f,0.15f,smog),1,
              0.22f,0.20f,0.15f,1);

    /* Smog cloud overlay */
    BlendOn();
    for(int c=0;c<8;c++){
        float cx2=c*170.0f+sinf(gt*0.1f+c)*20;
        float cy2=H*0.55f+c*20+cosf(gt*0.08f+c*0.7f)*15;
        Col4(0.45f,0.38f,0.25f,0.18f*smog*fa);
        Circle(cx2,cy2,90+c*15,24);
    }
    BlendOff();

    /* Ground */
    RectGradV(0,0,W,130, 0.28f,0.24f,0.18f,1, 0.35f,0.30f,0.22f,1);

    /* Road */
    Col3(0.22f,0.20f,0.18f);
    Rect(0,70,W,60);
    BlendOn();
    Col4(0.55f,0.50f,0.15f,0.5f);
    glLineWidth(2.0f);
    DashLine(0,100,W,100,40,20,gt*0.2f);
    BlendOff();

    /* Buildings */
    int nblds=9;
    float bx[]={20,130,240,360,460,570,690,810,960};
    float bw[]={95,90,100,85,90,100,95,110,200};
    float bh[]={220,310,260,380,290,340,270,400,180};
    for(int i=0;i<nblds;i++){
        /* Dirty building color */
        float dirt=0.15f+0.1f*(i%3);
        Col3(0.28f+dirt,0.25f+dirt,0.20f+dirt);
        Rect(bx[i],130,bw[i],bh[i]);
        /* Dark facade gradient */
        RectGradV(bx[i],130,bw[i],bh[i],
                  0.0f,0.0f,0.0f,0.3f,0.0f,0.0f,0.0f,0.0f);
        /* Windows */
        int wrows=bh[i]/30, wcols=bw[i]/22;
        for(int wr=0;wr<wrows;wr++){
            for(int wc=0;wc<wcols;wc++){
                float wx2=bx[i]+5+wc*22;
                float wy2=135+wr*30;
                float lit=sinf(gt*0.3f+i*1.1f+wr*0.5f+wc*0.3f)>0?1:0;
                Col3(lit*(0.8f+0.1f*frand()), lit*(0.7f+0.1f*frand()), lit*0.3f);
                Rect(wx2,wy2,14,18);
            }
        }
        /* Graffiti/dirt streaks */
        BlendOn();
        Col4(0.15f,0.12f,0.08f,0.3f);
        for(int st2=0;st2<3;st2++){
            float sx=bx[i]+10+st2*(bw[i]/3);
            Rect(sx,130,3,bh[i]*0.4f);
        }
        BlendOff();
    }

    /* GARBAGE PILES along street - growing with time */
    float pile_fill=ease_out(t);
    draw_garbage_pile(100,  130, 55, pile_fill*0.7f);
    draw_garbage_pile(280,  130, 65, pile_fill*0.9f);
    draw_garbage_pile(430,  130, 50, pile_fill*0.6f);
    draw_garbage_pile(580,  130, 70, pile_fill);
    draw_garbage_pile(720,  130, 60, pile_fill*0.8f);
    draw_garbage_pile(900,  130, 55, pile_fill*0.75f);
    draw_garbage_pile(1100, 130, 75, pile_fill*0.95f);

    /* Overflowing trash cans */
    for(int tc=0;tc<6;tc++){
        float tcx=80+tc*210.0f;
        float tcy=130;
        /* Shadow */
        drawShadow(tcx,tcy-2,18,8);
        /* Can body */
        Col3(0.35f,0.32f,0.28f);
        Rect(tcx-12,tcy,24,45);
        /* Overflow waste */
        if(pile_fill>0.3f){
            Col3(0.55f,0.45f,0.25f);
            float ov=(pile_fill-0.3f)/0.7f;
            for(int o=0;o<(int)(ov*6);o++){
                Circle(tcx+(-8+o*3),tcy+45+o*4,8+o*2,10);
            }
        }
        /* Lid (askew) */
        Col3(0.3f,0.28f,0.22f);
        glPushMatrix();
        glTranslatef(tcx,tcy+47,0);
        glRotatef(30*pile_fill*(tc%2?1:-1),0,0,1);
        Rect(-14,-4,28,8);
        glPopMatrix();
    }

    /* ─── STREETLIGHTS with glow ─── */
    draw_streetlight(160,130,150,1,fa);
    draw_streetlight(500,130,150,1,fa);
    draw_streetlight(840,130,150,1,fa);
    draw_streetlight(1120,130,150,1,fa);

    /* ─── PEOPLE SILHOUETTES ─── */
    BlendOn();
    for(int p=0;p<6;p++){
        float px=100+p*190.0f+sinf(gt*0.3f+p)*20;
        float walk_p=gt*3.0f+p*1.5f;
        float dir=(p%2)?1.0f:-1.0f;
        draw_person(px,130,walk_p,dir, 0.12f,0.10f,0.08f);
    }
    BlendOff();

    /* ─── RATS scurrying between piles ─── */
    for(int r=0;r<3;r++){
        float rat_x=fmodf(gt*45*(1+r*0.3f)+r*300,W+100)-50;
        draw_rat(rat_x,132,gt*8+r*2);
    }

    /* ─── WIND-BLOWN DEBRIS (paper, leaves) ─── */
    if(frand()<0.6f*pile_fill){
        float wx=frand()*W;
        SpawnParticleEx(wx,130+frand()*50,
                        30+frand()*40, frand()*30+10,
                        2.5f+frand()*2,
                        0.6f,0.5f,0.3f, 4+frand()*3,
                        3, 0.15f, -0.3f);  /* leaf type, slight upward float */
    }
    /* Dust from piles */
    if(frand()<0.4f*pile_fill){
        float dx=200+frand()*800;
        SpawnParticleEx(dx,130,
                        (frand()-0.5f)*15,frand()*8,
                        1.5f, 0.5f,0.45f,0.3f,3, 5,0.2f,0.0f);
    }
    DrawParticles();

    /* ─── BROKEN NEON SIGN ─── */
    BlendOn();
    float neon_flicker=sinf(gt*12)*sinf(gt*7.3f)*sinf(gt*3.1f);
    float neon_on=(neon_flicker>-0.2f)?1.0f:0.0f;
    neon_on*=clamp01(t*3); /* appears over time */
    Col4(0.8f*neon_on,0.1f,0.1f,0.7f*fa*neon_on);
    Text(730,440,"CLEAN CITY",GLUT_BITMAP_HELVETICA_18);
    if(neon_on>0.5f){
        drawGlow(790,445,60, 0.8f,0.1f,0.1f);
    }
    /* Strikethrough */
    if(t>0.4f){
        Col4(1.0f,0.2f,0.1f,0.9f*fa);
        glLineWidth(3.0f);
        Line2(720,448,870,448);
    }
    BlendOff();

    /* ─── ENHANCED SMOG LAYERS ─── */
    BlendOn();
    /* Layer 1: low, fast */
    for(int c=0;c<6;c++){
        float cx3=fmodf(c*220+gt*12,W+200)-100;
        Col4(0.4f,0.35f,0.20f,0.12f*smog*fa);
        Circle(cx3,H*0.4f+c*15,70+c*10,20);
    }
    /* Layer 2: high, slow */
    for(int c=0;c<5;c++){
        float cx4=fmodf(c*280+gt*5,W+200)-100;
        Col4(0.35f,0.30f,0.18f,0.08f*smog*fa);
        Circle(cx4,H*0.7f+c*20,100+c*20,20);
    }
    BlendOff();

    /* Stat counter: waste growing */
    BlendOn();
    Col4(0,0,0,0.65f*fa);
    Rect(W-300,H-130,290,120);
    Col4(1.0f,0.35f,0.15f,fa);
    Text(W-290,H-30,"WASTE CRISIS",GLUT_BITMAP_HELVETICA_18);
    Col4(0.95f,0.9f,0.85f,fa*0.9f);
    char buf[80];
    sprintf(buf,"City waste: %.0f tons/day",1200+2800*pile_fill);
    Text(W-290,H-60,buf,GLUT_BITMAP_HELVETICA_12);
    sprintf(buf,"Collection rate: %.0f%%",(1-pile_fill*0.4f)*100);
    Text(W-290,H-80,buf,GLUT_BITMAP_HELVETICA_12);
    Text(W-290,H-100,"Landfills: 94% FULL",GLUT_BITMAP_HELVETICA_12);
    Text(W-290,H-120,"Recycling rate: 8%",GLUT_BITMAP_HELVETICA_12);
    BlendOff();

    /* Label */
    BlendOn();
    Col4(1.0f,0.35f,0.15f,fa);
    Text(20,H-30,"SCENE 1: THE WASTE CRISIS",GLUT_BITMAP_HELVETICA_18);
    Col4(0.9f,0.85f,0.8f,fa*0.8f);
    Text(20,H-55,"Poor management  |  overflowing landfills  |  urban pollution",GLUT_BITMAP_HELVETICA_12);
    BlendOff();
}


/* ══════════════════════════════════════════════════════
   SCENE 2 — SMART BIN NETWORK & IoT SENSORS (55–105s)
   ══════════════════════════════════════════════════════ */
typedef struct{ float x,y,fill,fill_cap; int type; float signal_phase; } SmartBin;
static SmartBin sbins[16];
static int sbins_init=0;

static void init_sbins(void){
    if(sbins_init) return;
    float types_r[]={0.2f,0.85f,0.15f,0.2f};
    float types_g[]={0.75f,0.6f,0.7f,0.5f};
    float types_b[]={0.2f,0.1f,0.25f,0.9f};
    /* 4x4 grid layout */
    for(int i=0;i<16;i++){
        sbins[i].x=120+( i%4)*260.0f;
        sbins[i].y=110+(i/4)*130.0f;
        sbins[i].fill=0.2f+0.7f*(i%7)/6.0f;
        sbins[i].fill_cap=0.85f;
        sbins[i].type=i%4;
        sbins[i].signal_phase=i*0.4f;
    }
    sbins_init=1;
}

static const char* bin_type_name[]={ "ORGANIC","PLASTIC","PAPER","GLASS" };
static float bin_cr[]={0.2f,0.85f,0.15f,0.2f};
static float bin_cg[]={0.75f,0.6f,0.7f,0.5f};
static float bin_cb[]={0.2f,0.1f,0.25f,0.9f};

static void draw_smart_bin(SmartBin*b, float reveal){
    float x=b->x, y=b->y;
    int tp=b->type;
    float cr=bin_cr[tp], cg=bin_cg[tp], cb=bin_cb[tp];
    float fill=b->fill*reveal;
    float alert=(fill>0.75f)?1.0f:0.0f;

    /* Shadow */
    BlendOn();
    Col4(0,0,0,0.3f);
    Circle(x,y-2,28,20);
    BlendOff();

    /* Add bin wobble */
    float bin_wobble = alert ? sinf(gt*15+b->signal_phase)*3 : 0;
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(bin_wobble, 0, 0, 1);
    glTranslatef(-x, -y, 0);

    /* Bin body - trapezoid */
    glBegin(GL_QUADS);
    Col3(cr*0.5f,cg*0.5f,cb*0.5f);
    glVertex2f(x-22,y); glVertex2f(x+22,y);
    Col3(cr*0.7f,cg*0.7f,cb*0.7f);
    glVertex2f(x+18,y+55); glVertex2f(x-18,y+55);
    glEnd();

    /* Fill level indicator */
    float fh=50*fill;
    glBegin(GL_QUADS);
    Col3(cr,cg,cb);
    glVertex2f(x-18,y); glVertex2f(x+18,y);
    glVertex2f(x+14,y+fh); glVertex2f(x-14,y+fh);
    glEnd();

    /* Lid */
    float lid_angle= alert ? sinf(gt*8+b->signal_phase)*5 : 0;
    glPushMatrix();
    glTranslatef(x,y+56,0);
    glRotatef(lid_angle,0,0,1);
    Col3(cr*0.6f,cg*0.6f,cb*0.6f);
    Rect(-24,-5,48,10);
    glPopMatrix();

    /* IoT sensor on top */
    Col3(0.15f,0.15f,0.18f);
    Rect(x-6,y+58,12,14);
    /* Sensor LED */
    float led_pulse=0.5f+0.5f*sinf(gt*3+b->signal_phase);
    BlendOn();
    if(alert){
        Col4(1.0f,0.2f,0.1f,led_pulse);
    } else {
        Col4(0.2f,1.0f,0.3f,led_pulse);
    }
    Circle(x,y+68,4,8);
    BlendOff();

    /* WiFi signal waves */
    BlendOn();
    for(int w=1;w<=3;w++){
        float wa=clamp01(sinf(gt*1.5f+b->signal_phase-w*0.4f));
        Col4(0.3f,0.8f,1.0f,wa*0.5f*reveal);
        float ang1=PI*0.65f, ang2=PI*0.35f;
        float wr=w*12.0f;
        glLineWidth(1.5f);
        glBegin(GL_LINE_STRIP);
        for(int seg=0;seg<=12;seg++){
            float a=ang2+(ang1-ang2)*seg/12.0f;
            glVertex2f(x+cosf(a)*wr,y+72+sinf(a)*wr);
        }
        glEnd();
    }
    BlendOff();

    /* Fill % label */
    BlendOn();
    Col4(1,1,1,0.9f*reveal);
    char pct[8]; sprintf(pct,"%.0f%%",fill*100);
    Text(x-10,y+28,pct,GLUT_BITMAP_HELVETICA_10);

    /* Alert flash */
    if(alert && reveal>0.5f){
        float af=0.5f+0.5f*sinf(gt*6+b->signal_phase);
        Col4(1.0f,0.2f,0.1f,af*0.3f);
        Ring(x,y+28,35,4,20);
    }
    BlendOff();
}

static void draw_iot_network(void){
    float t=st(T2,T3);
    float fa=fade_alpha(T2,T3);

    init_sbins();

    /* Clean city background */
    RectGradV(0,0,W,H, 0.08f,0.12f,0.16f,1, 0.04f,0.06f,0.10f,1);

    /* Grid pattern */
    BlendOn();
    glLineWidth(1.0f);
    for(int gx=0;gx<W;gx+=60){
        Col4(0.15f,0.22f,0.30f,0.3f);
        Line2(gx,0,gx,H);
    }
    for(int gy=0;gy<H;gy+=60){
        Col4(0.15f,0.22f,0.30f,0.3f);
        Line2(0,gy,W,gy);
    }
    BlendOff();

    /* Central server icon — enhanced */
    float srv_x=W/2.0f, srv_y=H*0.85f;
    drawShadow(srv_x,srv_y-22,50,12);
    Col3(0.18f,0.22f,0.28f);
    Rect(srv_x-40,srv_y-20,80,40);
    /* Server rack detail */
    Col3(0.12f,0.15f,0.20f);
    for(int sr=0;sr<3;sr++) Rect(srv_x-38,srv_y-18+sr*12,76,10);
    /* Server LEDs — expanded array */
    for(int sl=0;sl<8;sl++){
        float sp=0.5f+0.5f*sinf(gt*3+sl*0.6f);
        Col3(0.1f,sp*0.8f,0.3f);
        Circle(srv_x-32+sl*9,srv_y+5,3,6);
    }
    /* Cooling fan animation */
    glPushMatrix();
    glTranslatef(srv_x+25,srv_y-5,0);
    glRotatef(gt*300,0,0,1);
    Col3(0.3f,0.35f,0.4f);
    for(int fb=0;fb<4;fb++){
        float fa2=TAU*fb/4;
        Line2(0,0,cosf(fa2)*10,sinf(fa2)*10);
    }
    glPopMatrix();

    drawGlow(srv_x,srv_y,60, 0.1f,0.3f,0.6f);
    BlendOn();
    Col4(0.3f,0.7f,1.0f,0.8f);
    Text(srv_x-38,srv_y+28,"CITY SERVER",GLUT_BITMAP_HELVETICA_10);

    /* Radar sweep on server */
    {float sweep_a=fmodf(gt*1.5f,1.0f)*TAU;
    Col4(0.2f,0.6f,1.0f,0.15f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(srv_x,srv_y);
    for(int rs=0;rs<=12;rs++){
        float a=sweep_a-0.5f+0.5f*rs/12.0f;
        glVertex2f(srv_x+cosf(a)*55,srv_y+sinf(a)*55);
    }
    glEnd();}
    BlendOff();

    /* ─── MESH NETWORK lines between neighboring bins ─── */
    BlendOn();
    glLineWidth(0.8f);
    for(int i=0;i<16;i++){
        for(int j=i+1;j<16;j++){
            float dx=sbins[i].x-sbins[j].x;
            float dy=sbins[i].y-sbins[j].y;
            float dist=sqrtf(dx*dx+dy*dy);
            if(dist<300){
                float mesh_alpha=0.08f*(1-dist/300)*smoothstep(t)*fa;
                Col4(0.2f,0.5f,0.8f,mesh_alpha);
                Line2(sbins[i].x,sbins[i].y+35,sbins[j].x,sbins[j].y+35);
            }
        }
    }
    BlendOff();

    /* Data lines from bins to server */
    float reveal_t=smoothstep(t);
    for(int i=0;i<16;i++){
        SmartBin*b=&sbins[i];
        float bin_reveal=smoothstep(clamp01(t*16-i*0.5f));
        if(bin_reveal<0.01f) continue;

        /* Connection line */
        BlendOn();
        float data_pulse=fmodf(gt*1.2f+i*0.25f,1.0f);
        Col4(0.2f,0.6f,1.0f,0.15f*bin_reveal*fa);
        glLineWidth(1.0f);
        Line2(b->x,b->y+35,srv_x,srv_y);

        /* Data packet traveling — enhanced with glow trail */
        if(b->fill>0.5f){
            float px=lerpf(b->x,srv_x,data_pulse);
            float py=lerpf(b->y+35,srv_y,data_pulse);
            /* Trail */
            for(int trail=3;trail>=1;trail--){
                float tp=clamp01(data_pulse-trail*0.04f);
                float tx=lerpf(b->x,srv_x,tp);
                float ty=lerpf(b->y+35,srv_y,tp);
                Col4(0.2f,0.7f,1.0f,0.2f*(4-trail)/3.0f*bin_reveal*fa);
                Circle(tx,ty,3,6);
            }
            /* Main packet */
            Col4(0.3f,0.9f,1.0f,0.9f*(1-data_pulse)*bin_reveal*fa);
            Circle(px,py,5,8);
            Col4(0.3f,0.9f,1.0f,0.3f*(1-data_pulse)*bin_reveal*fa);
            Circle(px,py,10,10);
        }
        BlendOff();

        /* Draw bin */
        glPushMatrix();
        float scale=bin_reveal;
        glTranslatef(b->x,b->y+28,0);
        glScalef(scale,scale,1);
        glTranslatef(-b->x,-(b->y+28),0);
        draw_smart_bin(b,bin_reveal*fa);
        glPopMatrix();

        /* Type label below bin */
        BlendOn();
        int tp=b->type;
        Col4(bin_cr[tp],bin_cg[tp],bin_cb[tp],0.8f*bin_reveal*fa);
        Text(b->x-18,b->y-10,bin_type_name[tp],GLUT_BITMAP_HELVETICA_10);
        BlendOff();
    }


    /* Legend panel */
    BlendOn();
    Col4(0,0,0,0.7f*fa);
    Rect(W-280,H-190,270,175);
    Col4(0.3f,0.8f,1.0f,fa);
    Text(W-270,H-30,"IoT SMART BIN LEGEND",GLUT_BITMAP_HELVETICA_12);

    const char* legends[]={ "ORGANIC — Food / Garden",
                             "PLASTIC — Bottles / Bags",
                             "PAPER — Cardboard / News",
                             "GLASS — Bottles / Jars" };
    for(int i=0;i<4;i++){
        Col4(bin_cr[i],bin_cg[i],bin_cb[i],fa);
        Rect(W-270,H-65-i*28,16,16);
        Col4(0.9f,0.9f,0.9f,fa*0.9f);
        Text(W-248,H-52-i*28,legends[i],GLUT_BITMAP_HELVETICA_10);
    }

    /* Alert legend */
    Col4(1.0f,0.2f,0.1f,fa);
    Circle(W-262,H-185,6,8);
    Col4(0.8f,0.8f,0.8f,fa*0.8f);
    Text(W-248,H-180,"RED = >75% full → alert sent",GLUT_BITMAP_HELVETICA_10);
    BlendOff();

    /* Stats popup (appears mid-scene) */
    if(t>0.5f){
        float sa=smoothstep((t-0.5f)/0.5f)*fa;
        BlendOn();
        Col4(0,0,0,0.75f*sa);
        Rect(20,H-200,310,185);
        Col4(0.3f,1.0f,0.5f,sa);
        Text(30,H-30,"SMART BIN NETWORK",GLUT_BITMAP_HELVETICA_18);
        Col4(0.85f,0.95f,0.85f,sa*0.9f);
        Text(30,H-60,"Total smart bins deployed: 1,240",GLUT_BITMAP_HELVETICA_12);
        Text(30,H-80,"Avg fill monitored: real-time",GLUT_BITMAP_HELVETICA_12);
        Text(30,H-100,"Collection alerts sent: auto",GLUT_BITMAP_HELVETICA_12);
        Text(30,H-120,"Sensor accuracy: 99.2%",GLUT_BITMAP_HELVETICA_12);
        Text(30,H-140,"Energy: solar-powered",GLUT_BITMAP_HELVETICA_12);
        Text(30,H-160,"Data refresh: every 15 min",GLUT_BITMAP_HELVETICA_12);
        Text(30,H-180,"Overflow incidents: -78%",GLUT_BITMAP_HELVETICA_12);
        BlendOff();
    }

    BlendOn();
    Col4(1,1,1,fa);
    Text(20,H-220,"SCENE 2: SMART BIN NETWORK & IoT SENSORS",GLUT_BITMAP_HELVETICA_18);
    BlendOff();
}

/* ══════════════════════════════════════════════════════
   SCENE 3 — AUTOMATED COLLECTION TRUCKS & ROUTES (105–155s)
   ══════════════════════════════════════════════════════ */
#define NTRUCK 4
typedef struct{
    float x,y,angle;
    float speed;
    int   route_pt;
    float route_t;
    float r,g,b;
    const char* label;
} Truck;

static Truck trucks[NTRUCK];
static int trucks_init=0;

/* Route waypoints (circular route per truck) */
static float route[][2][8]={
    /* truck 0: outer ring */
    {{80,300,900,1200,1200,900,200,80},
     {130,500,600,500,130,130,130,130}},
    /* truck 1: inner upper */
    {{400,700,900,700,400,200,400,400},
     {420,500,420,350,350,420,420,420}},
    /* truck 2: inner lower */
    {{300,600,900,600,300,300,300,300},
     {250,320,250,180,180,250,250,250}},
    /* truck 3: right wing */
    {{900,1100,1100,900,700,900,900,900},
     {200,300,500,580,500,300,200,200}},
};

static void init_trucks(void){
    if(trucks_init) return;
    float cr[]={0.2f,0.85f,0.15f,0.95f};
    float cg[]={0.8f,0.6f,0.7f,0.7f};
    float cb[]={0.2f,0.1f,0.25f,0.1f};
    const char* lbl[]={"ORGANIC","PLASTIC","PAPER","MIXED"};
    for(int i=0;i<NTRUCK;i++){
        trucks[i].x=route[i][0][0];
        trucks[i].y=route[i][1][0];
        trucks[i].angle=0;
        trucks[i].speed=50+i*15;
        trucks[i].route_pt=0;
        trucks[i].route_t=i*0.25f;
        trucks[i].r=cr[i];
        trucks[i].g=cg[i];
        trucks[i].b=cb[i];
        trucks[i].label=lbl[i];
    }
    trucks_init=1;
}

static void update_trucks(float dt){
    int nwp=8;
    for(int i=0;i<NTRUCK;i++){
        Truck*tr=&trucks[i];
        int wp=tr->route_pt;
        int nxt=(wp+1)%nwp;
        float tx=route[i][0][nxt], ty=route[i][1][nxt];
        float dx=tx-tr->x, dy=ty-tr->y;
        float dist=sqrtf(dx*dx+dy*dy);
        if(dist<5){
            tr->route_pt=nxt;
        } else {
            float spd=tr->speed*dt;
            tr->x+=dx/dist*spd;
            tr->y+=dy/dist*spd;
            tr->angle=atan2f(dy,dx)*180/PI;
        }
        /* Exhaust particles */
        if(frand()<0.3f){
            float ea=tr->angle*PI/180+PI;
            SpawnParticle(tr->x+cosf(ea)*20,tr->y+sinf(ea)*20,
                          cosf(ea)*10+frand()*10-5,
                          sinf(ea)*10+frand()*10+5,
                          1.2f, 0.6f,0.6f,0.6f,5);
        }
    }
}

static void draw_truck_vehicle(float x,float y,float angle,
                                float cr,float cg,float cb,const char*lbl){
    glPushMatrix();
    glTranslatef(x,y,0);
    glRotatef(angle,0,0,1);

    /* Truck body */
    Col3(cr*0.7f,cg*0.7f,cb*0.7f);
    Rect(-30,-12,60,24);
    /* Cab */
    Col3(cr,cg,cb);
    Rect(15,-11,22,22);
    /* Windshield */
    Col3(0.5f,0.8f,1.0f);
    Rect(22,-8,12,16);
    /* Waste container */
    Col3(cr*0.5f,cg*0.5f,cb*0.5f);
    Rect(-28,-10,44,20);
    /* Wheels */
    Col3(0.15f,0.15f,0.15f);
    Circle(-18,-14,8,12);
    Circle(18,-14,8,12);
    Circle(-18,14,8,12);
    Circle(18,14,8,12);
    Col3(0.4f,0.4f,0.4f);
    Circle(-18,-14,4,8);
    Circle(18,-14,4,8);
    Circle(-18,14,4,8);
    Circle(18,14,4,8);
    /* Headlights */
    BlendAdd();
    Col4(1.0f,1.0f,0.6f,0.8f);
    Circle(34,-7,5,8);
    Circle(34,7,5,8);
    BlendOff();

    glPopMatrix();

    /* Label */
    BlendOn();
    Col4(cr,cg,cb,0.9f);
    Text(x-18,y+20,lbl,GLUT_BITMAP_HELVETICA_10);
    BlendOff();
}

static void draw_routes(void){
    /* Draw route paths */
    float cr2[]={0.2f,0.85f,0.15f,0.95f};
    float cg2[]={0.8f,0.6f,0.7f,0.7f};
    float cb2[]={0.2f,0.1f,0.25f,0.1f};
    int nwp=8;
    for(int i=0;i<NTRUCK;i++){
        BlendOn();
        glLineWidth(1.5f);
        float phase=fmodf(gt*0.3f+i*0.25f,1.0f);
        for(int wp=0;wp<nwp;wp++){
            int nxt=(wp+1)%nwp;
            Col4(cr2[i],cg2[i],cb2[i],0.25f);
            Line2(route[i][0][wp],route[i][1][wp],
                  route[i][0][nxt],route[i][1][nxt]);
            /* Animated dots */
            Col4(cr2[i],cg2[i],cb2[i],0.6f);
            float px=lerpf(route[i][0][wp],route[i][0][nxt],phase);
            float py=lerpf(route[i][1][wp],route[i][1][nxt],phase);
            Circle(px,py,3,8);
        }
        BlendOff();
    }
}

static void draw_collection_scene(void){
    float t=st(T3,T4);
    float fa=fade_alpha(T3,T4);

    init_trucks();
    if(!paused) update_trucks(1.0f/fps);

    /* City map style background */
    Col3(0.10f,0.13f,0.10f);
    Rect(0,0,W,H);

    /* City blocks (greens = parks, grays = buildings) */
    /* Parks */
    Col3(0.12f,0.20f,0.11f);
    Rect(130,200,250,200); Rect(430,330,220,160);
    Rect(730,160,200,220); Rect(980,280,200,180);
    /* Building blocks */
    Col3(0.18f,0.18f,0.22f);
    Rect(130,420,200,150); Rect(370,160,180,160);
    Rect(660,400,200,160); Rect(900,130,180,130);

    /* Road grid */
    Col3(0.22f,0.22f,0.25f);
    /* Main roads */
    Rect(0,120,W,30);  /* bottom road */
    Rect(0,380,W,30);  /* mid road */
    Rect(0,550,W,30);  /* upper road */
    Rect(80,0,30,H);
    Rect(360,0,30,H);
    Rect(640,0,30,H);
    Rect(920,0,30,H);
    Rect(1150,0,30,H);

    /* Road markings */
    BlendOn();
    Col4(0.7f,0.65f,0.1f,0.5f);
    glLineWidth(1.5f);
    DashLine(0,135,W,135,25,15,gt*0.15f);
    DashLine(0,395,W,395,25,15,gt*0.12f);
    DashLine(0,565,W,565,25,15,gt*0.18f);
    BlendOff();

    /* ─── TRAFFIC LIGHTS at intersections ─── */
    draw_traffic_light(95+15,120,fa);
    draw_traffic_light(375+15,380,fa);
    draw_traffic_light(655+15,120,fa);
    draw_traffic_light(935+15,380,fa);

    /* ─── CIVILIAN CARS on roads ─── */
    {float car_colors[][3]={{0.6f,0.15f,0.15f},{0.15f,0.15f,0.6f},{0.5f,0.5f,0.15f},{0.15f,0.5f,0.15f},{0.5f,0.3f,0.1f}};
    for(int cv=0;cv<5;cv++){
        float cx=fmodf(gt*35*(1+cv*0.2f)+cv*250,W+100)-50;
        float cy=(cv<2)?135:(cv<4)?395:565;
        float dir=(cv%2)?1.0f:-1.0f;
        draw_civilian_car(cx,cy,dir,car_colors[cv][0],car_colors[cv][1],car_colors[cv][2]);
    }}

    /* Smart bins on streets */
    init_sbins();
    for(int i=0;i<16;i++){
        SmartBin*b=&sbins[i];
        float bx=b->x, by=b->y;
        BlendOn();
        Col4(bin_cr[b->type],bin_cg[b->type],bin_cb[b->type],0.8f*fa);
        Rect(bx-6,by,12,18);
        Col4(bin_cr[b->type]*0.5f,bin_cg[b->type]*0.5f,bin_cb[b->type]*0.5f,0.9f);
        Rect(bx-6,by,12,18*b->fill);
        if(b->fill>BIN_ALERT_THRESHOLD){
            float ag=0.5f+0.5f*sinf(gt*4+i);
            Col4(1.0f,0.2f,0.1f,ag*0.6f);
            Ring(bx,by+9,12,2,12);
        }
        BlendOff();
    }

    /* Draw routes */
    draw_routes();

    /* ─── GPS TRAIL behind trucks ─── */
    BlendOn();
    for(int i=0;i<NTRUCK;i++){
        for(int trail=12;trail>=1;trail--){
            float tp=gt-trail*0.15f;
            if(tp<T3) continue;
            /* Approximate past position using current direction */
            float tx=trucks[i].x-cosf(trucks[i].angle*PI/180)*trail*5;
            float ty=trucks[i].y-sinf(trucks[i].angle*PI/180)*trail*5;
            float ta=0.4f*(1-trail/12.0f)*fa;
            Col4(trucks[i].r,trucks[i].g,trucks[i].b,ta);
            Circle(tx,ty,2.5f,6);
        }
    }
    BlendOff();

    /* Draw trucks — with suspension bounce */
    for(int i=0;i<NTRUCK;i++){
        float bounce=sinf(gt*12+i*2.5f)*SECONDARY_MOTION_AMP;
        draw_truck_vehicle(trucks[i].x,trucks[i].y+bounce,trucks[i].angle,
                           trucks[i].r,trucks[i].g,trucks[i].b,trucks[i].label);
    }
    DrawParticles();


    /* Collection event animations */
    for(int i=0;i<16;i++){
        SmartBin*b=&sbins[i];
        /* Check if truck is near */
        for(int ti=0;ti<NTRUCK;ti++){
            float dx=trucks[ti].x-b->x, dy=trucks[ti].y-b->y;
            if(dx*dx+dy*dy<1600){
                BlendAdd();
                float ca=0.6f+0.4f*sinf(gt*5);
                Col4(0.5f,1.0f,0.5f,ca);
                Circle(b->x,b->y+9,18,12);
                BlendOff();
            }
        }
    }

    /* Info panel */
    BlendOn();
    Col4(0,0,0,0.72f*fa);
    Rect(W-300,H-200,288,185);
    Col4(0.5f,1.0f,0.5f,fa);
    Text(W-290,H-30,"FLEET MANAGEMENT",GLUT_BITMAP_HELVETICA_18);
    Col4(0.9f,0.95f,0.9f,fa*0.9f);
    Text(W-290,H-58,"Active trucks: 4 / 12",GLUT_BITMAP_HELVETICA_12);
    Text(W-290,H-76,"Fuel type: CNG + Electric",GLUT_BITMAP_HELVETICA_12);
    Text(W-290,H-94,"Route optimization: AI",GLUT_BITMAP_HELVETICA_12);
    Text(W-290,H-112,"Collections today: 38",GLUT_BITMAP_HELVETICA_12);
    Text(W-290,H-130,"CO2 saved vs old: -42%",GLUT_BITMAP_HELVETICA_12);
    Text(W-290,H-148,"Avg route time: 2.1h",GLUT_BITMAP_HELVETICA_12);
    Text(W-290,H-166,"GPS tracked: YES",GLUT_BITMAP_HELVETICA_12);
    Text(W-290,H-184,"Citizen alerts: ACTIVE",GLUT_BITMAP_HELVETICA_12);

    Col4(1,1,1,fa);
    Text(20,H-30,"SCENE 3: AUTOMATED COLLECTION ROUTES",GLUT_BITMAP_HELVETICA_18);
    Col4(0.8f,0.95f,0.8f,fa*0.8f);
    Text(20,H-55,"AI-optimized routes · Electric trucks · Real-time tracking",GLUT_BITMAP_HELVETICA_12);
    BlendOff();
}

/* ══════════════════════════════════════════════════════
   SCENE 4 — SORTING FACILITY & RECYCLING (155–200s)
   ══════════════════════════════════════════════════════ */
#define NSORTING_ITEMS 30
typedef struct{ float x,y,vx,vy; int type; float size; int sorted; } SortItem;
static SortItem sitems[NSORTING_ITEMS];
static int sitems_init=0;

static void init_sitems(void){
    if(sitems_init) return;
    for(int i=0;i<NSORTING_ITEMS;i++){
        sitems[i].x=frand()*200-100;
        sitems[i].y=200+frand()*100;
        sitems[i].vx=30+frand()*40;
        sitems[i].vy=0;
        sitems[i].type=i%4;
        sitems[i].size=8+frand()*8;
        sitems[i].sorted=0;
    }
    sitems_init=1;
}

static void draw_sorting_facility(void){
    float t=st(T4,T5);
    float fa=fade_alpha(T4,T5);

    init_sitems();

    /* Industrial background */
    RectGradV(0,0,W,H, 0.12f,0.12f,0.15f,1, 0.08f,0.08f,0.10f,1);

    /* Factory floor */
    Col3(0.22f,0.22f,0.25f);
    Rect(0,0,W,110);
    /* Floor tiles */
    BlendOn();
    for(int fx=0;fx<W;fx+=80){
        Col4(0.3f,0.3f,0.35f,0.3f);
        Line2(fx,0,fx,110);
    }
    for(int fy=0;fy<110;fy+=40){
        Col4(0.3f,0.3f,0.35f,0.3f);
        Line2(0,fy,W,fy);
    }
    BlendOff();

    /* ─── CONVEYOR BELT SYSTEM ─── */
    /* Main conveyor belt */
    float belt_y=200;
    Col3(0.3f,0.25f,0.15f);
    Rect(0,belt_y-20,W,40);
    /* Belt texture */
    glLineWidth(2.0f);
    float belt_phase=fmodf(gt*60,40);
    for(float bx=-belt_phase;bx<W;bx+=40){
        BlendOn();
        Col4(0.4f,0.32f,0.20f,0.6f);
        Line2(bx,belt_y-20,bx,belt_y+20);
        BlendOff();
    }
    /* Belt rollers */
    for(int rx=0;rx<W;rx+=120){
        Col3(0.45f,0.40f,0.35f);
        Rect(rx,belt_y-22,20,44);
        Col3(0.6f,0.55f,0.50f);
        Circle(rx+10,belt_y,18,12);
        Col3(0.4f,0.35f,0.30f);
        Circle(rx+10,belt_y,10,12);
        /* Spinning animation */
        float spoke_a=gt*4+rx;
        glLineWidth(2.5f);
        Col3(0.7f,0.65f,0.60f);
        Line2(rx+10+cosf(spoke_a)*9,belt_y+sinf(spoke_a)*9,
              rx+10-cosf(spoke_a)*9,belt_y-sinf(spoke_a)*9);
        Line2(rx+10+cosf(spoke_a+PI/2)*9,belt_y+sinf(spoke_a+PI/2)*9,
              rx+10-cosf(spoke_a+PI/2)*9,belt_y-sinf(spoke_a+PI/2)*9);
    }

    /* ─── SORTING ARMS ─── */
    float arm_x[]={280,530,780,1030};
    float arm_cr[]={0.2f,0.9f,0.15f,0.2f};
    float arm_cg[]={0.8f,0.6f,0.7f,0.5f};
    float arm_cb[]={0.2f,0.1f,0.25f,0.9f};
    const char* arm_label[]={"ORGANIC","PLASTIC","PAPER","GLASS"};

    for(int a=0;a<4;a++){
        float ax=arm_x[a];
        float arm_angle=25.0f*sinf(gt*1.5f+a*PI/2);
        /* Arm mount */
        Col3(0.35f,0.35f,0.38f);
        Rect(ax-10,belt_y+20,20,60);
        /* Arm */
        glPushMatrix();
        glTranslatef(ax,belt_y+70,0);
        glRotatef(arm_angle,0,0,1);
        Col3(arm_cr[a],arm_cg[a],arm_cb[a]);
        Rect(-8,0,16,50);
        /* Grabber */
        Col3(arm_cr[a]*0.8f,arm_cg[a]*0.8f,arm_cb[a]*0.8f);
        Rect(-14,50,28,12);
        glPopMatrix();

        /* Label */
        BlendOn();
        Col4(arm_cr[a],arm_cg[a],arm_cb[a],0.9f*fa);
        Text(ax-20,belt_y+135,arm_label[a],GLUT_BITMAP_HELVETICA_10);
        BlendOff();
    }

    /* ─── COLLECTION BINS BELOW BELT ─── */
    float bin_y=80;
    for(int b=0;b<4;b++){
        float bx=arm_x[b];
        /* Bin */
        Col3(arm_cr[b]*0.5f,arm_cg[b]*0.5f,arm_cb[b]*0.5f);
        glBegin(GL_QUADS);
        glVertex2f(bx-45,bin_y); glVertex2f(bx+45,bin_y);
        glVertex2f(bx+35,bin_y+80); glVertex2f(bx-35,bin_y+80);
        glEnd();
        /* Fill */
        float fill_level=clamp01(t*1.5f-b*0.1f);
        glBegin(GL_QUADS);
        Col3(arm_cr[b],arm_cg[b],arm_cb[b]);
        glVertex2f(bx-43,bin_y); glVertex2f(bx+43,bin_y);
        glVertex2f(bx+33,bin_y+78*fill_level); glVertex2f(bx-33,bin_y+78*fill_level);
        glEnd();
        /* Label */
        BlendOn();
        Col4(1,1,1,0.8f*fa);
        char pct2[12]; sprintf(pct2,"%.0f%%",fill_level*100);
        Text(bx-10,bin_y+40,pct2,GLUT_BITMAP_HELVETICA_12);
        BlendOff();
    }

    /* ─── WASTE ITEMS ON BELT ─── */
    float belt_phase2=fmodf(gt*60,W);
    /* Draw moving waste items */
    int item_shapes[][2]={ {0,0},{1,0},{2,0},{3,0},{0,1},{1,1},{2,1},{3,1} };
    for(int it=0;it<NSORTING_ITEMS;it++){
        float ix=fmodf(belt_phase2+it*(W/NSORTING_ITEMS),W);
        float iy=belt_y + sinf(gt*20+it*3.1f)*1.5f; /* item vibration */
        int tp=it%4;
        Col3(bin_cr[tp],bin_cg[tp],bin_cb[tp]);
        /* Different shapes for different waste */
        switch(tp){
            case 0: /* organic - irregular blob */
                Circle(ix,iy,8+3*sinf(it),8); break;
            case 1: /* plastic - bottle shape */
                Rect(ix-4,iy-10,8,18);
                Circle(ix,iy-10,5,8); break;
            case 2: /* paper - rectangle */
                Rect(ix-7,iy-5,14,10); break;
            case 3: /* glass - bottle */
                Rect(ix-4,iy-12,8,20);
                Circle(ix,iy-12,4,8); break;
        }
        /* AI Detection overlay when under sorting arm */
        for(int a=0;a<4;a++){
            if(fabsf(ix-arm_x[a])<30 && a==tp){
                /* Detection box */
                BlendOn();
                Col4(arm_cr[a],arm_cg[a],arm_cb[a],0.7f);
                glLineWidth(2.0f);
                /* Animated box corners */
                float bsz=18+2*sinf(gt*6);
                /* Top-left corner */
                Line2(ix-bsz,iy+bsz,ix-bsz+8,iy+bsz);
                Line2(ix-bsz,iy+bsz,ix-bsz,iy+bsz-8);
                /* Top-right */
                Line2(ix+bsz,iy+bsz,ix+bsz-8,iy+bsz);
                Line2(ix+bsz,iy+bsz,ix+bsz,iy+bsz-8);
                /* Bottom-left */
                Line2(ix-bsz,iy-bsz,ix-bsz+8,iy-bsz);
                Line2(ix-bsz,iy-bsz,ix-bsz,iy-bsz+8);
                /* Bottom-right */
                Line2(ix+bsz,iy-bsz,ix+bsz-8,iy-bsz);
                Line2(ix+bsz,iy-bsz,ix+bsz,iy-bsz+8);
                /* Detection label */
                Col4(arm_cr[a],arm_cg[a],arm_cb[a],0.9f);
                char det_buf[32];
                sprintf(det_buf,"AI: %s",arm_label[a]);
                Text(ix-18,iy+bsz+8,det_buf,GLUT_BITMAP_HELVETICA_10);
                BlendAdd();
                Col4(arm_cr[a],arm_cg[a],arm_cb[a],0.3f);
                Circle(ix,iy,bsz*0.8f,12);
                BlendOff();
            }
        }
    }

    /* ─── SCANNING LINE across conveyor ─── */
    BlendOn();
    float scan_x=fmodf(gt*120,W);
    Col4(0.2f,1.0f,0.3f,0.4f);
    glLineWidth(1.5f);
    Line2(scan_x,belt_y-25,scan_x,belt_y+25);
    Col4(0.2f,1.0f,0.3f,0.1f);
    Rect(scan_x-15,belt_y-25,30,50);
    BlendOff();

    /* ─── FACILITY STRUCTURE ─── */
    /* Ceiling / roof */
    Col3(0.18f,0.18f,0.22f);
    Rect(0,H-80,W,80);
    /* Support pillars with safety beacons */
    for(int p=0;p<7;p++){
        Col3(0.25f,0.25f,0.30f);
        Rect(p*200-10,110,20,H-180);
        Col3(0.40f,0.40f,0.45f);
        Rect(p*200-15,H-90,30,10);
        /* Rotating amber safety beacon */
        BlendOn();
        float beacon_phase=gt*4+p*1.2f;
        float beacon_bright=clamp01(sinf(beacon_phase));
        Col4(1.0f,0.7f,0.1f,beacon_bright*0.6f);
        Circle(p*200,H-85,6,8);
        if(beacon_bright>0.5f){
            drawGlow(p*200,H-85,25, 1.0f,0.7f,0.1f);
        }
        BlendOff();
    }
    /* Overhead crane */
    glLineWidth(3.0f);
    float crane_x=200+fmodf(gt*40,W-400);
    Col3(0.5f,0.5f,0.55f);
    Rect(crane_x-5,H-80,10,200);
    BlendOn();
    Col4(1.0f,0.8f,0.2f,0.8f);
    glLineWidth(5.0f);
    Line2(0,H-70,W,H-70);
    BlendOff();

    /* Sparks from machinery — upgrade to spark type */
    if(frand()<0.4f){
        float sx=arm_x[(int)(frand()*4)];
        for(int sp=0;sp<3;sp++){
            SpawnParticleEx(sx,belt_y+70,
                          (frand()-0.5f)*80,(frand())*60+20,
                          0.5f, 1.0f,0.8f,0.2f,3, 2,0.2f,1.0f);
        }
    }
    DrawParticles();


    /* ─── RECYCLING STATS ─── */
    BlendOn();
    Col4(0,0,0,0.72f*fa);
    Rect(20,H-220,320,205);
    Col4(0.5f,1.0f,0.5f,fa);
    Text(30,H-30,"RECYCLING METRICS",GLUT_BITMAP_HELVETICA_18);
    Col4(0.9f,0.95f,0.9f,fa*0.9f);
    char rbuf[80];
    float recycled=42+30*t;
    sprintf(rbuf,"Recycled today: %.0f tonnes",recycled);
    Text(30,H-58,rbuf,GLUT_BITMAP_HELVETICA_12);
    Text(30,H-76,"Sorting accuracy: 96.8%",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-94,"Processing speed: 12 t/hr",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-112,"Items sorted per hour: 3,400",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-130,"AI sorter error rate: 0.3%",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-148,"Contamination detected: AUTO",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-166,"Workers onsite: 8 (monitoring)",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-184,"Facility hours: 24/7",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-202,"Revenue from recyclables: +24%",GLUT_BITMAP_HELVETICA_12);

    Col4(1,1,1,fa);
    Text(360,H-30,"SCENE 4: SORTING FACILITY & RECYCLING",GLUT_BITMAP_HELVETICA_18);
    BlendOff();
}

/* ══════════════════════════════════════════════════════
   SCENE 5 — WASTE-TO-ENERGY PLANT (200–245s)
   ══════════════════════════════════════════════════════ */
static float fa_flag=1.0f; /* fade alpha passed into draw_flame */

static void draw_flame(float cx,float cy,float h,float w,float phase){
    BlendAdd();
    int layers=6;
    for(int i=layers;i>=1;i--){
        float ff=(float)i/layers;
        float fw=w*(0.3f+0.7f*ff)*(0.9f+0.1f*sinf(gt*8+phase+i));
        float fh=h*ff*(0.9f+0.1f*sinf(gt*6+phase*1.3f+i*0.5f));
        float t2=1.0f-ff;
        /* Color: white→yellow→orange→red */
        float cr2=lerpf(1.0f,1.0f,t2);
        float cg2=lerpf(1.0f,0.0f,t2);
        float cb2=lerpf(0.8f,0.0f,t2);
        Col4(cr2,cg2,cb2,0.15f*fa_flag+0.0f);
        Col4(cr2,cg2,cb2, (i==1)?0.8f:0.4f/i );
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx,cy+fh);
        int segs=16;
        for(int s=0;s<=segs;s++){
            float a=PI*s/segs;
            float noise=1+0.15f*sinf(a*4+gt*7+phase+i);
            glVertex2f(cx+cosf(a)*fw*noise,cy+sinf(a)*fh*0.3f*noise);
        }
        glEnd();
    }
    BlendOff();
}

static void draw_smoke_plume(float cx, float base_y){
    BlendOn();
    for(int s=0;s<20;s++){
        float age=(fmodf(gt*0.4f+s*0.12f,1.0f));
        float py=base_y+age*300;
        float px=cx+sinf(age*4+s*0.6f)*40*age;
        float sz=10+age*60;
        float gray=0.55f+age*0.25f;
        Col4(gray,gray,gray,0.15f*(1-age));
        Circle(px,py,sz,16);
    }
    BlendOff();
}

static void draw_energy_plant(void){
    float t=st(T5,T6);
    float fa=fade_alpha(T5,T6);
    fa_flag=fa;

    /* Night sky - glowing from plant */
    RectGradV(0,0,W,H, 0.04f,0.06f,0.08f,1, 0.02f,0.03f,0.05f,1);
    /* Orange glow on horizon */
    BlendAdd();
    RectGradV(0,0,W,200, 0.0f,0.0f,0.0f,0, 0.3f,0.12f,0.0f,0.3f*fa);
    BlendOff();

    /* Stars and City Skyline */
    srand(99);
    for(int s=0;s<120;s++){
        float sx=(float)(s*137%W),sy=250+(float)(s*97%450);
        float tw=0.3f+0.7f*fabsf(sinf(gt*1.5f+s*0.4f));
        BlendOn();
        Col4(1,1,0.9f,tw*0.6f*fa);
        glPointSize(tw*2.5f);
        glBegin(GL_POINTS); glVertex2f(sx,sy); glEnd();
        BlendOff();
    }

    /* ─── CITY SKYLINE (Background) ─── */
    BlendOn();
    for(int b=0;b<30;b++){
        float bx=b*45.0f;
        float bw=30+sinf(b*123.456f)*20;
        float bh=80+sinf(b*987.654f)*100;
        /* Distant haze fades the buildings in */
        Col4(0.06f,0.07f,0.09f,0.8f*fa);
        Rect(bx,80,bw,bh);
        /* Simple windows in skyline */
        for(int wy=100;wy<80+bh-10;wy+=15){
            if(sinf(b*12+wy*4)>0.5f){
                Col4(0.8f,0.6f,0.2f,0.3f*fa);
                Rect(bx+bw*0.3f,wy,bw*0.4f,8);
            }
        }
    }
    BlendOff();

    /* Ground */
    RectGradV(0,0,W,80, 0.15f,0.14f,0.12f,1, 0.20f,0.18f,0.15f,1);

    /* ─── PLANT STRUCTURE ─── */
    /* Foundation */
    Col3(0.25f,0.24f,0.22f);
    Rect(200,80,800,50);

    /* Main building */
    RectGradV(250,130,300,280, 0.28f,0.26f,0.24f,1, 0.35f,0.33f,0.30f,1);
    RectGradV(580,130,180,220, 0.30f,0.28f,0.26f,1, 0.38f,0.36f,0.33f,1);
    RectGradV(780,130,170,180, 0.28f,0.26f,0.24f,1, 0.35f,0.33f,0.30f,1);

    /* Windows with orange glow */
    for(int wr=0;wr<5;wr++){
        for(int wc=0;wc<4;wc++){
            float wglow=0.5f+0.5f*sinf(gt*0.5f+wr*0.4f+wc*0.3f);
            Col3(wglow,wglow*0.5f,0.0f);
            Rect(265+wc*65,145+wr*48,45,30);
            if (wglow > 0.8f) drawGlow(265+wc*65+22, 145+wr*48+15, 30, wglow, wglow*0.5f, 0.0f);
        }
    }
    for(int wr=0;wr<4;wr++){
        for(int wc=0;wc<2;wc++){
            float wglow=0.4f+0.6f*sinf(gt*0.6f+wr*0.5f+wc*0.4f);
            Col3(wglow,wglow*0.4f,0.0f);
            Rect(595+wc*70,145+wr*48,45,30);
        }
    }

    /* ─── INCINERATOR CHIMNEYS ─── */
    float chimney_x[]={320,470,640,820};
    float chimney_h[]={320,380,340,300};
    for(int ch=0;ch<4;ch++){
        float cx=chimney_x[ch];
        float ch2=chimney_h[ch];
        /* Chimney body */
        RectGradH(cx-22,80,44,ch2, 0.35f,0.32f,0.28f, 0.22f,0.20f,0.18f);
        /* Chimney top ring */
        Col3(0.45f,0.42f,0.38f);
        Rect(cx-26,80+ch2-8,52,12);

        /* Flames at base */
        draw_flame(cx,80+30,80,35,ch*1.2f);

        /* Smoke plumes from top */
        draw_smoke_plume(cx,80+ch2+12);
        
        /* Ember particles rising from chimneys */
        if(frand()<0.3f){
            SpawnParticleEx(cx+(frand()-0.5f)*20, 80+ch2,
                            (frand()-0.5f)*10, 40+frand()*40, 
                            2.0f+frand()*2.0f,
                            1.0f, 0.5f+frand()*0.3f, 0.0f, 3+frand()*3,
                            6, 0.0f, -0.4f); /* ember type, float up */
        }
    }

    /* ─── TURBINE GENERATORS ─── */
    for(int tg=0;tg<3;tg++){
        float tx=300+tg*220.0f;
        float ty=155;
        /* Generator housing */
        drawShadow(tx+60,ty-10,60,15);
        Col3(0.4f,0.38f,0.35f);
        Rect(tx,ty,120,60);
        Col3(0.50f,0.48f,0.44f);
        Rect(tx+10,ty+10,100,40);
        
        /* Spinning turbine */
        glPushMatrix();
        glTranslatef(tx+60,ty+30,0);
        glRotatef(gt*300,0,0,1);
        Col3(0.6f,0.6f,0.65f);
        for(int bl=0;bl<6;bl++){
            float ba=TAU*bl/6;
            glBegin(GL_QUADS);
            float bx0=cosf(ba)*8,  by0=sinf(ba)*8;
            float bx1=cosf(ba)*25, by1=sinf(ba)*25;
            float bn=ba+0.5f;
            glVertex2f(bx0,by0);
            glVertex2f(cosf(bn)*8,sinf(bn)*8);
            glVertex2f(cosf(bn)*25,sinf(bn)*25);
            glVertex2f(bx1,by1);
            glEnd();
        }
        glPopMatrix();
        
        /* Energy glow inside turbine */
        drawGlow(tx+60,ty+30, 40, 0.4f,0.8f,1.0f);

        /* Output cables */
        glLineWidth(3.0f);
        BlendOn();
        Col4(1.0f,0.9f,0.1f,0.7f);
        Line2(tx+60,ty,tx+60,ty-40);
        Line2(tx+60,ty-40,tx+200,ty-40);
        /* Electricity arc effect */
        glLineWidth(2.0f);
        Col4(0.5f,0.8f,1.0f,0.6f+0.4f*pulse(8+tg));
        glBegin(GL_LINE_STRIP);
        for(int ep=0;ep<=10;ep++){
            float ex=tx+60+(140.0f*ep/10);
            float ey=ty-40+sinf(gt*20+ep*0.8f+tg)*8;
            glVertex2f(ex,ey);
        }
        glEnd();
        BlendOff();
    }

    /* ─── ENERGY OUTPUT DISPLAY ─── */
    /* Power line tower */
    Col3(0.4f,0.38f,0.35f);
    Rect(980,80,15,260);
    Rect(960,250,55,8);
    Rect(960,320,55,8);
    
    /* Wires */
    BlendOn();
    Col4(0.5f,0.5f,0.55f,0.8f);
    glLineWidth(1.5f);
    Line2(995,345,W,380);
    Line2(995,325,W,360);
    Line2(995,258,W,280);
    
    /* Energy flow particles along wire */
    for(int ef=0;ef<8;ef++){
        float ep=fmodf(gt*1.5f+ef*0.125f,1.0f);
        float ex=lerpf(995,W,ep);
        float ey=lerpf(345,380,ep);
        drawGlow(ex,ey, 25, 0.4f,0.8f,1.0f);
        Col4(0.8f,1.0f,1.0f,1-ep);
        Circle(ex,ey,6,8);
    }
    for(int ef=0;ef<8;ef++){
        float ep=fmodf(gt*1.3f+ef*0.125f,1.0f);
        float ex=lerpf(995,W,ep);
        float ey=lerpf(325,360,ep);
        drawGlow(ex,ey, 20, 0.4f,0.8f,1.0f);
    }
    for(int ef=0;ef<8;ef++){
        float ep=fmodf(gt*1.7f+ef*0.125f,1.0f);
        float ex=lerpf(995,W,ep);
        float ey=lerpf(258,280,ep);
        drawGlow(ex,ey, 20, 0.4f,0.8f,1.0f);
    }
    BlendOff();
    
    DrawParticles();

    /* ─── INFO PANEL ─── */
    BlendOn();
    Col4(0,0,0,0.75f*fa);
    Rect(20,H-240,330,225);
    Col4(1.0f,0.6f,0.1f,fa);
    Text(30,H-30,"WASTE-TO-ENERGY PLANT",GLUT_BITMAP_HELVETICA_18);
    Col4(0.95f,0.90f,0.85f,fa*0.9f);
    char ebuf[80];
    float mw=18+12*t;
    sprintf(ebuf,"Energy generated: %.1f MW",mw);
    Text(30,H-60,ebuf,GLUT_BITMAP_HELVETICA_12);
    sprintf(ebuf,"Waste burned: %.0f t/day",320+180*t);
    Text(30,H-80,ebuf,GLUT_BITMAP_HELVETICA_12);
    Text(30,H-100,"Homes powered: 14,000+",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-120,"Emission filters: ACTIVE",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-140,"Air quality compliance: 100%",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-160,"Carbon capture: 85%",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-180,"Ash reuse (construction): YES",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-200,"Efficiency rating: A+",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-220,"Landfill diversion: 94%",GLUT_BITMAP_HELVETICA_12);

    Col4(1,1,1,fa);
    Text(400,H-30,"SCENE 5: WASTE-TO-ENERGY PLANT",GLUT_BITMAP_HELVETICA_18);
    BlendOff();

    DrawParticles();
}

/* ══════════════════════════════════════════════════════
   SCENE 6 — CITY DASHBOARD & ANALYTICS (245–275s)
   ══════════════════════════════════════════════════════ */
static void draw_arc_gauge(float cx,float cy,float r,float val,
                            float cr,float cg,float cb,
                            const char*label,const char*value){
    /* BG arc */
    BlendOn();
    Col4(0.15f,0.15f,0.18f,0.8f);
    Ring(cx,cy,r,10,48);
    /* Value arc */
    float end_a=PI+PI*clamp01(val);
    Col4(cr,cg,cb,0.9f);
    glBegin(GL_QUAD_STRIP);
    int steps=48;
    for(int i=0;i<=steps*val;i++){
        float a=PI+PI*i/(float)steps;
        glVertex2f(cx+cosf(a)*(r-10),cy+sinf(a)*(r-10));
        glVertex2f(cx+cosf(a)*r,     cy+sinf(a)*r);
    }
    glEnd();
    /* Glow tip */
    if(val>0.05f){
        Col4(cr,cg,cb,0.5f);
        float ta=PI+PI*val;
        Circle(cx+cosf(ta)*r,cy+sinf(ta)*r,6,8);
    }
    /* Label */
    Col4(0.7f,0.7f,0.7f,0.9f);
    TextC(cx,cy-r-18,label,GLUT_BITMAP_HELVETICA_10);
    /* Value center */
    Col4(cr,cg,cb,1.0f);
    TextC(cx,cy-8,value,GLUT_BITMAP_HELVETICA_18);
    BlendOff();
}

static void draw_mini_bar(float x,float y,float w,float h,float val,
                           float cr,float cg,float cb){
    Col3(0.12f,0.12f,0.15f);
    Rect(x,y,w,h);
    Col3(cr,cg,cb);
    Rect(x,y,w*clamp01(val),h);
    BlendOn();
    Col4(1,1,1,0.5f);
    Rect(x,y+h-3,w*clamp01(val),3);
    BlendOff();
}

static void draw_line_graph_s6(float x,float y,float w,float h,
                                float*data,int n,float cr,float cg,float cb){
    /* Background */
    Col3(0.08f,0.09f,0.12f);
    Rect(x,y,w,h);
    /* Grid */
    BlendOn();
    glLineWidth(1.0f);
    for(int g=1;g<5;g++){
        Col4(0.2f,0.22f,0.28f,0.5f);
        Line2(x,y+h*g/5.0f,x+w,y+h*g/5.0f);
    }
    /* Line */
    glLineWidth(2.5f);
    Col4(cr,cg,cb,0.9f);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<n;i++){
        glVertex2f(x+w*i/(n-1.0f), y+h*clamp01(data[i]));
    }
    glEnd();
    /* Area fill */
    Col4(cr,cg,cb,0.12f);
    glBegin(GL_QUADS);
    for(int i=0;i<n-1;i++){
        glVertex2f(x+w*i/(n-1.0f),     y);
        glVertex2f(x+w*(i+1)/(n-1.0f), y);
        glVertex2f(x+w*(i+1)/(n-1.0f), y+h*clamp01(data[i+1]));
        glVertex2f(x+w*i/(n-1.0f),     y+h*clamp01(data[i]));
    }
    glEnd();
    BlendOff();
}

static void draw_dashboard(void){
    float t=st(T6,T7);
    float fa=fade_alpha(T6,T7);

    /* Dark dashboard background */
    Col3(0.04f,0.05f,0.08f);
    Rect(0,0,W,H);

    /* Subtle hex pattern */
    BlendOn();
    for(int hx=0;hx<18;hx++){
        for(int hy=0;hy<12;hy++){
            float hcx=hx*74+(hy%2)*37;
            float hcy=hy*62;
            Col4(0.10f,0.15f,0.20f,0.2f);
            CircleOutline(hcx,hcy,28,6,0.8f);
        }
    }
    BlendOff();

    /* Panel dividers */
    BlendOn();
    glLineWidth(1.5f);
    Col4(0.15f,0.3f,0.4f,0.6f);
    Line2(0,H*0.55f,W,H*0.55f);
    Line2(W*0.5f,H*0.55f,W*0.5f,0);
    Line2(W*0.25f,H*0.55f,W*0.25f,H);
    Line2(W*0.5f,H*0.55f,W*0.5f,H);
    Line2(W*0.75f,H*0.55f,W*0.75f,H);
    BlendOff();

    /* ─── CLOCK WIDGET (Center Hub) ─── */
    BlendOn();
    Col4(0.08f,0.1f,0.15f,0.9f*fa);
    Circle(W/2.0f, H*0.55f, 40, 32);
    Col4(0.2f,0.8f,1.0f,0.8f*fa);
    CircleOutline(W/2.0f, H*0.55f, 40, 32, 3.0f);
    /* Clock ticks */
    for(int tk=0;tk<12;tk++){
        float tka=TAU*tk/12;
        Line2(W/2.0f+cosf(tka)*32, H*0.55f+sinf(tka)*32,
              W/2.0f+cosf(tka)*38, H*0.55f+sinf(tka)*38);
    }
    /* Clock hands */
    glLineWidth(2.5f);
    Col4(1.0f,1.0f,1.0f,0.9f*fa);
    Line2(W/2.0f, H*0.55f, W/2.0f+cosf(gt*0.5f)*20, H*0.55f+sinf(gt*0.5f)*20); /* hour */
    glLineWidth(1.5f);
    Col4(0.3f,0.9f,1.0f,0.9f*fa);
    Line2(W/2.0f, H*0.55f, W/2.0f+cosf(gt*6.0f)*32, H*0.55f+sinf(gt*6.0f)*32);  /* minute */
    BlendOff();

    /* ─── LEFT PANEL: Arc Gauges ─── */
    float fill_anim = smoothstep((t - 0.1f) * 3.0f); /* animate 0->1 */
    float gauge_vals[]={
        0.62f+0.2f*sinf(gt*0.3f),
        0.78f+0.1f*sinf(gt*0.4f+1),
        0.55f+0.2f*cosf(gt*0.35f+2),
        0.88f+0.05f*sinf(gt*0.5f+3),
    };
    const char* glabels[]={"RECYCLING","DIVERSION","RECOVERY","EFFICIENCY"};
    char gvals[4][12];
    sprintf(gvals[0],"%.0f%%",gauge_vals[0]*fill_anim*100);
    sprintf(gvals[1],"%.0f%%",gauge_vals[1]*fill_anim*100);
    sprintf(gvals[2],"%.0f%%",gauge_vals[2]*fill_anim*100);
    sprintf(gvals[3],"%.0f%%",gauge_vals[3]*fill_anim*100);
    float gcx[]={120,300,480,W/2.0f-60};
    float gcr2[]={0.2f,0.9f,0.15f,0.4f};
    float gcg2[]={0.9f,0.6f,0.8f,0.8f};
    float gcb2[]={0.2f,0.1f,0.2f,1.0f};
    for(int g=0;g<4;g++){
        draw_arc_gauge(gcx[g],H*0.3f,70,gauge_vals[g] * fill_anim,
                       gcr2[g],gcg2[g],gcb2[g],glabels[g],gvals[g]);
    }

    /* ─── RIGHT PANEL: Line graphs ─── */
    float waste_data[12]={ 0.9f,0.85f,0.82f,0.78f,0.72f,0.68f,0.62f,0.55f,0.48f,0.40f,0.32f,0.25f };
    float recycle_data[12]={ 0.1f,0.15f,0.18f,0.25f,0.32f,0.38f,0.45f,0.52f,0.60f,0.68f,0.75f,0.82f };
    
    /* Progressive drawing of graphs */
    int graph_pts = (int)(fill_anim * 12);
    if(graph_pts < 2) graph_pts = 2; /* Need at least 2 points to draw line */
    if(graph_pts > 12) graph_pts = 12;

    float gx=W*0.52f, gy=H*0.05f, gw=W*0.45f, gh=H*0.22f;
    draw_line_graph_s6(gx,gy+gh*1.3f,gw,gh*0.9f,waste_data,graph_pts,1.0f,0.35f,0.1f);
    draw_line_graph_s6(gx,gy,gw,gh*0.9f,recycle_data,graph_pts,0.2f,0.9f,0.3f);

    BlendOn();
    Col4(1.0f,0.35f,0.1f,0.9f);
    Rect(gx,gy+gh*2.35f,12,10);
    Col4(0.9f,0.9f,0.9f,0.8f);
    Text(gx+16,gy+gh*2.35f+8,"Waste Generated (trend ↓)",GLUT_BITMAP_HELVETICA_10);
    Col4(0.2f,0.9f,0.3f,0.9f);
    Rect(gx+230,gy+gh*2.35f,12,10);
    Col4(0.9f,0.9f,0.9f,0.8f);
    Text(gx+246,gy+gh*2.35f+8,"Recycling Rate (trend ↑)",GLUT_BITMAP_HELVETICA_10);

    /* Graph titles */
    Col4(0.6f,0.7f,0.8f,0.9f);
    Text(gx+4,gy+gh*1.3f+gh*0.9f+12,"WEEKLY WASTE TREND",GLUT_BITMAP_HELVETICA_10);
    Text(gx+4,gy+gh*0.9f+12,"RECYCLING RATE TREND",GLUT_BITMAP_HELVETICA_10);
    BlendOff();

    /* ─── TOP PANELS ─── */
    struct{ const char*name; float val; float cr,cg,cb; const char*unit; } kpis[]={
        {"Landfill Diversion",  0.92f, 0.2f,0.9f,0.3f, "92%"},
        {"Carbon Footprint",    0.28f, 0.4f,0.7f,1.0f, "-72%"},
        {"Citizen Compliance",  0.85f, 0.9f,0.8f,0.2f, "85%"},
        {"Compost Produced",    0.68f, 0.5f,0.9f,0.2f, "68t"},
        {"Biogas Generated",    0.74f, 1.0f,0.6f,0.1f, "74%"},
        {"App Active Users",    0.91f, 0.3f,0.7f,1.0f, "91K"},
        {"Cost Savings",        0.55f, 0.9f,0.5f,0.9f, "55%"},
        {"Zero-Waste Zones",    0.40f, 0.2f,1.0f,0.8f, "40%"},
    };
    int nkpis=8;
    for(int k=0;k<nkpis;k++){
        float kx=W*0.25f*(k%4), ky=(k<4)?H*0.57f:H*0.78f;
        float kw=W*0.25f, kh=H*0.20f;
        /* Panel bg */
        BlendOn();
        Col4(0.06f,0.08f,0.12f,0.9f*fa);
        Rect(kx+5,ky+5,kw-10,kh-10);
        /* Title */
        Col4(0.6f,0.65f,0.72f,fa*0.8f);
        Text(kx+15,ky+kh-15,kpis[k].name,GLUT_BITMAP_HELVETICA_10);
        /* Bar */
        draw_mini_bar(kx+15,ky+kh*0.48f,kw-30,14,
                      kpis[k].val*(0.9f+0.1f*sinf(gt*0.5f+k)),
                      kpis[k].cr,kpis[k].cg,kpis[k].cb);
        /* Value */
        Col4(kpis[k].cr,kpis[k].cg,kpis[k].cb,fa);
        Text(kx+15,ky+kh*0.2f,kpis[k].unit,GLUT_BITMAP_HELVETICA_18);
        BlendOff();
    }

    /* Title bar */
    BlendOn();
    Col4(0,0,0,0.7f*fa);
    Rect(0,H-36,W,36);
    Col4(0.3f,1.0f,0.6f,fa);
    TextC(W/2.0f,H-20,"SCENE 6: SMART CITY WASTE MANAGEMENT DASHBOARD",GLUT_BITMAP_HELVETICA_18);
    BlendOff();
}

/* ══════════════════════════════════════════════════════
   SCENE 7 — ZERO-WASTE FUTURE VISION (275–300s)
   ══════════════════════════════════════════════════════ */
static void draw_tree(float x,float y,float h,float lush){
    /* Trunk */
    Col3(0.35f,0.22f,0.08f);
    Rect(x-5,y,10,h*0.4f);
    /* Canopy layers */
    for(int l=0;l<4;l++){
        float lt=(float)l/3;
        float lr=lerpf(0.08f,0.15f,lt+0.1f*sinf(gt*0.5f+x));
        float lg=lerpf(0.45f,0.75f,lt*lush);
        float lb=lerpf(0.05f,0.12f,lt);
        Col3(lr,lg,lb);
        float rr=(h*0.35f-l*h*0.06f)*(0.9f+0.1f*sinf(gt*0.8f+x*0.02f+l));
        float sway = sinf(gt * 1.5f + x * 0.05f + l*0.5f) * l * 3.0f; /* Canopy sway */
        Circle(x+sway, y+h*0.4f+l*h*0.15f, rr, 16);
    }
}

static void draw_solar_panel(float x,float y,float w,float h){
    float tilt_angle = 5 * sinf(gt * 0.5f); /* Sun tracking subtle tilt */
    glPushMatrix();
    glTranslatef(x+w/2, y+h/2, 0);
    glRotatef(tilt_angle, 0, 0, 1);
    glTranslatef(-(x+w/2), -(y+h/2), 0);

    Col3(0.08f,0.10f,0.30f);
    Rect(x,y,w,h);
    /* Grid lines */
    glLineWidth(1.0f);
    Col3(0.15f,0.18f,0.45f);
    for(int c=1;c<4;c++) Line2(x+w*c/4,y,x+w*c/4,y+h);
    for(int r=1;r<3;r++) Line2(x,y+h*r/3,x+w,y+h*r/3);
    /* Sheen */
    BlendAdd();
    float sh=0.1f+0.1f*pulse(0.5f);
    Col4(0.4f,0.5f,1.0f,sh);
    Rect(x,y+h*0.3f,w,h*0.1f);
    BlendOff();
    glPopMatrix();
}

static void draw_future_city(void){
    float t=st(T7,T8);
    float fa=fade_alpha(T7,T8);

    /* Bright clean sky */
    RectGradV(0,0,W,H,
              lerpf(0.35f,0.55f,t),lerpf(0.60f,0.80f,t),lerpf(0.80f,0.95f,t),1,
              0.65f,0.85f,0.65f,1);

    /* Rainbow arc appearing */
    draw_rainbow(W/2.0f,150,550,clamp01(t*2)*fa);

    /* Sun */
    BlendAdd();
    Col4(1.0f,0.95f,0.7f,0.3f);
    Circle(W-150,H-80,80,32);
    Col4(1.0f,0.95f,0.6f,0.8f);
    Circle(W-150,H-80,45,32);
    /* Sun rays */
    glLineWidth(2.0f);
    for(int ray=0;ray<12;ray++){
        float ra=gt*0.2f+TAU*ray/12;
        Col4(1.0f,0.9f,0.5f,0.4f);
        Line2(W-150+cosf(ra)*50,H-80+sinf(ra)*50,
              W-150+cosf(ra)*90,H-80+sinf(ra)*90);
    }
    BlendOff();

    /* Birds in V-formation */
    BlendOn();
    float bx_base=fmodf(gt*60,W+300)-150;
    float by_base=H*0.75f+sinf(gt)*20;
    for(int bird=0;bird<7;bird++){
        float bx_off=bird*15;
        float by_off=abs(bird-3)*15;
        draw_bird(bx_base-bx_off,by_base-by_off,gt*8+bird*0.5f);
    }
    BlendOff();

    /* Clouds */
    BlendOn();
    for(int cl=0;cl<5;cl++){
        float clx=fmodf(cl*280+gt*15,W+200)-100;
        float cly=H*0.55f+cl*25;
        Col4(1.0f,1.0f,1.0f,0.7f);
        Circle(clx,cly,40,20);
        Circle(clx+35,cly-10,30,20);
        Circle(clx-35,cly-5,28,20);
        Circle(clx+20,cly+12,25,20);
    }
    BlendOff();

    /* Green hills */
    glBegin(GL_TRIANGLE_FAN);
    Col3(0.20f,0.58f,0.22f);
    glVertex2f(W/2.0f,0);
    for(int s=0;s<=50;s++){
        float a=PI*s/50;
        glVertex2f(cosf(a)*W*0.8f+W*0.5f,sinf(a)*160);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    Col3(0.15f,0.48f,0.18f);
    glVertex2f(-100,0);
    for(int s=0;s<=30;s++){
        float a=PI*s/30;
        glVertex2f(-100+cosf(a)*500,sinf(a)*120);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    Col3(0.18f,0.52f,0.20f);
    glVertex2f(W+50,0);
    for(int s=0;s<=30;s++){
        float a=PI+PI*s/30;
        glVertex2f(W+50+cosf(a)*480,sinf(a)*100);
    }
    glEnd();

    /* Wind turbines on hills */
    draw_wind_turbine(250,140,80,gt*2.0f);
    draw_wind_turbine(450,150,90,gt*2.0f+PI/3);
    draw_wind_turbine(850,145,100,gt*2.0f+PI/6);
    draw_wind_turbine(1050,135,70,gt*2.0f+PI/2);
    
    /* Ground */
    RectGradV(0,0,W,90, 0.25f,0.60f,0.25f,1, 0.35f,0.72f,0.30f,1);

    /* Pathways */
    Col3(0.75f,0.72f,0.65f);
    Rect(0,60,W,22);
    Rect(320,0,25,110);
    Rect(680,0,25,110);
    Rect(1000,0,25,110);

    /* ─── BUILDINGS: FUTURE ECO TOWERS ─── */
    struct{ float x,w,h; float cr,cg,cb; } blds[]={
        {40, 90,320, 0.7f,0.85f,0.75f},
        {150,75,420, 0.65f,0.80f,0.70f},
        {240,65,280, 0.75f,0.88f,0.78f},
        {350,80,510, 0.60f,0.82f,0.72f},
        {460,70,360, 0.70f,0.86f,0.76f},
        {560,90,460, 0.65f,0.83f,0.73f},
        {680,75,320, 0.72f,0.87f,0.77f},
        {790,85,440, 0.62f,0.81f,0.71f},
        {910,70,380, 0.68f,0.84f,0.74f},
        {1020,80,280,0.73f,0.88f,0.78f},
        {1120,90,350,0.66f,0.82f,0.72f},
    };
    for(int b=0;b<11;b++){
        float bx=blds[b].x, bw=blds[b].w, bh=blds[b].h;
        float bcr=blds[b].cr,bcg=blds[b].cg,bcb=blds[b].cb;
        /* Building */
        RectGradV(bx,90,bw,bh, bcr*0.7f,bcg*0.7f,bcb*0.7f,1, bcr,bcg,bcb,1);
        /* Glass facade */
        BlendOn();
        RectGradH(bx,90,bw,bh, 0.8f,0.95f,1.0f, 0.6f,0.85f,0.95f);
        BlendOff();
        /* Windows (clean, modern) */
        int wrows=(int)(bh/35), wcols=(int)(bw/22);
        for(int wr=0;wr<wrows;wr++){
            for(int wc=0;wc<wcols;wc++){
                float bright=0.8f+0.2f*sinf(gt*0.3f+b+wr+wc);
                Col3(bright*0.7f,bright*0.85f,bright);
                Rect(bx+4+wc*22,95+wr*35,14,22);
            }
        }
        /* Rooftop garden */
        Col3(0.15f,0.55f,0.20f);
        Rect(bx+5,90+bh,bw-10,12);
        /* Small trees on roof */
        for(int rt=0;rt<(int)(bw/30);rt++){
            Col3(0.12f,0.50f,0.18f);
            Circle(bx+15+rt*28,90+bh+18,8,8);
        }
        /* Solar panels on roof */
        draw_solar_panel(bx+5,90+bh+2,bw-10,8);

        /* Vertical garden strips */
        BlendOn();
        for(int vs=0;vs<3;vs++){
            float vx=bx+vs*(bw/3);
            Col4(0.15f,0.6f,0.2f,0.4f+0.1f*sinf(gt*0.5f+b+vs));
            Rect(vx,90,8,bh);
        }
        BlendOff();
    }

    /* Trees lining the street */
    for(int tr=0;tr<12;tr++){
        float tx=60+tr*105.0f;
        draw_tree(tx,70,80,0.9f);
    }

    /* ─── SOLAR FARM in background ─── */
    for(int sp=0;sp<8;sp++){
        float spx=50+sp*150.0f;
        float spy=200;
        Col3(0.3f,0.28f,0.25f);
        Line2(spx+20,spy,spx+20,spy+50);
        draw_solar_panel(spx,spy+50,40,28);
        BlendAdd();
        float ssh=0.05f+0.05f*pulse(0.5f+sp*0.1f);
        Col4(0.5f,0.6f,1.0f,ssh);
        Rect(spx,spy+50,40,28);
        BlendOff();
    }

    /* ─── ELECTRIC VEHICLES ─── */
    for(int ev=0;ev<3;ev++){
        float evx=fmodf(gt*40*(1+ev*0.3f)+ev*400,W+100)-50;
        float evy=65;
        /* EV body */
        Col3(0.3f,0.75f,0.4f+ev*0.1f);
        glBegin(GL_QUADS);
        glVertex2f(evx,evy);
        glVertex2f(evx+60,evy);
        glVertex2f(evx+55,evy+18);
        glVertex2f(evx+5,evy+18);
        glEnd();
        /* Cabin */
        Col3(0.6f,0.88f,0.95f);
        Rect(evx+12,evy+18,32,14);
        /* Wheels */
        Col3(0.15f,0.15f,0.15f);
        Circle(evx+12,evy,8,10);
        Circle(evx+48,evy,8,10);
        /* EV glow */
        BlendAdd();
        Col4(0.2f,1.0f,0.4f,0.2f);
        Circle(evx+12,evy,12,10);
        Circle(evx+48,evy,12,10);
        BlendOff();
    }

    /* ─── WASTE BINS (zero-waste style, clean) ─── */
    float clean_bin_x[]={100,350,600,850,1100};
    for(int cb2=0;cb2<5;cb2++){
        float cbx=clean_bin_x[cb2];
        /* Modern clean bin */
        Col3(0.3f,0.85f,0.4f);
        Rect(cbx-10,68,20,30);
        Col3(0.2f,0.70f,0.3f);
        Rect(cbx-12,95,24,6);
        /* Nearly empty (success!) */
        Col3(0.15f,0.55f,0.2f);
        Rect(cbx-9,68,18,5);
        /* Recycling symbol */
        BlendOn();
        Col4(1.0f,1.0f,1.0f,0.8f);
        glLineWidth(1.5f);
        for(int arrow=0;arrow<3;arrow++){
            float aa=TAU*arrow/3+gt*0.5f;
            float aa2=aa+TAU/3;
            glBegin(GL_LINE_STRIP);
            for(int as=0;as<=8;as++){
                float aang=aa+TAU/3*as/8.0f;
                glVertex2f(cbx+cosf(aang)*8,78+sinf(aang)*8);
            }
            glEnd();
        }
        BlendOff();
    }

    /* ─── STATS: FUTURE ACHIEVED ─── */
    BlendOn();
    Col4(0.0f,0.15f,0.05f,0.8f*fa);
    Rect(20,H-260,360,245);
    Col4(0.2f,1.0f,0.5f,fa);
    Text(30,H-25,"ZERO-WASTE CITY: 2035",GLUT_BITMAP_HELVETICA_18);
    Col4(0.85f,0.98f,0.87f,fa*0.95f);
    Text(30,H-55,"✓ Recycling Rate: 92%",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-75,"✓ Landfill Usage: ZERO",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-95,"✓ Waste-to-Energy: 6% residuals",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-115,"✓ Composting: 100% organics",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-135,"✓ Carbon neutral: ACHIEVED",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-155,"✓ Green cover: +45% increase",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-175,"✓ Air Quality Index: EXCELLENT",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-195,"✓ Smart bins: 12,000+ deployed",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-215,"✓ Electric fleet: 100% trucks",GLUT_BITMAP_HELVETICA_12);
    Text(30,H-235,"✓ Citizen App users: 2.4 Million",GLUT_BITMAP_HELVETICA_12);

    /* Closing credits */
    if(t>0.55f){
        float ca=smoothstep((t-0.55f)/0.45f)*fa;
        Col4(0,0.05f,0,0.85f*ca);
        Rect(W/2.0f-280,H/2.0f-80,560,160);
        Col4(0.3f,1.0f,0.5f,ca);
        TextC(W/2.0f,H/2.0f+55,"THE PATH TO ZERO WASTE",GLUT_BITMAP_TIMES_ROMAN_24);
        Col4(0.9f,0.95f,0.9f,ca*0.9f);
        TextC(W/2.0f,H/2.0f+22,"Smart Bins · AI Routes · Automated Sorting",GLUT_BITMAP_HELVETICA_18);
        TextC(W/2.0f,H/2.0f-2,"Waste-to-Energy · City Analytics · Green Future",GLUT_BITMAP_HELVETICA_18);
        Col4(0.5f,0.8f,0.5f,ca*0.7f);
        TextC(W/2.0f,H/2.0f-40,"[R] Replay  |  [Q] Quit",GLUT_BITMAP_HELVETICA_12);
    }

    /* Floating leaf particles */
    if(frand()<0.5f){
        float lx=frand()*W;
        SpawnParticle(lx,0,
                      (frand()-0.5f)*20,60+frand()*40,
                      4.0f+frand()*3,
                      0.1f+frand()*0.2f, 0.5f+frand()*0.3f, 0.1f,
                      4+frand()*4);
    }
    DrawParticles();

    Col4(1,1,1,fa);
    Text(20,H-280,"SCENE 7: THE ZERO-WASTE FUTURE",GLUT_BITMAP_HELVETICA_18);
    BlendOff();
}

/* ══════════════════════════════════════════════════════
   HUD OVERLAY (always on top)
   ══════════════════════════════════════════════════════ */
static const char* scene_names[]={
    "INTRO","WASTE CRISIS","SMART BINS & IoT",
    "COLLECTION ROUTES","SORTING FACILITY",
    "WASTE-TO-ENERGY","CITY DASHBOARD","ZERO-WASTE FUTURE"
};
static const float scene_starts[]={ T0,T1,T2,T3,T4,T5,T6,T7,T8 };

static void draw_hud(void){
    BlendOn();

    /* Top banner */
    Col4(0,0,0,0.55f);
    Rect(0,H-32,W,32);

    /* Scene name */
    if(scene>=0 && scene<=7){
        Col4(0.4f,0.9f,0.5f,0.9f);
        char sname[80];
        sprintf(sname,"SCENE %d: %s",scene,scene_names[scene]);
        Text(20,H-20,sname,GLUT_BITMAP_HELVETICA_12);
    }

    /* Time display */
    int ts=(int)gt, te=(int)(T8-gt);
    char tbuf[32];
    sprintf(tbuf,"%02d:%02d  |  -%02d:%02d left",ts/60,ts%60,te/60,te%60);
    Text(W-230,H-20,tbuf,GLUT_BITMAP_HELVETICA_12);

    /* FPS */
    char fbuf[20]; sprintf(fbuf,"%.0f FPS",fps);
    Col4(0.5f,0.5f,0.5f,0.6f);
    Text(W-80,12,fbuf,GLUT_BITMAP_HELVETICA_10);

    /* Progress bar */
    float prog=gt/T8;
    /* Progress bar background */
    Col4(0,0,0,0.6f);
    Rect(0,0,W,10);
    /* Gradient progress */
    float pr=lerpf(0.9f,0.2f,prog), pg=lerpf(0.3f,1.0f,prog), pb=0.2f;
    Col4(pr,pg,pb,0.9f);
    Rect(0,0,W*prog,10);
    /* Glow tip */
    BlendAdd();
    Col4(pr,pg,pb,0.5f);
    Rect(W*prog-6,0,12,10);
    BlendOff();

    /* Scene thumbnail strip/timeline */
    for(int sc=0;sc<8;sc++){
        float st = scene_starts[sc];
        float en = (sc == 7) ? T8 : scene_starts[sc+1];
        float t_start = W * st / T8;
        float t_end = W * en / T8;
        float t_width = t_end - t_start;

        if(sc == scene){
            /* Highlight active scene timeline chunk */
            BlendAdd();
            Col4(1.0f, 1.0f, 1.0f, 0.25f + 0.15f*sinf(gt*5));
            Rect(t_start, 0, t_width, 10);
            BlendOff();
        }

        /* Scene dividers */
        BlendOn();
        Col4(0.0f, 0.0f, 0.0f, 0.8f);
        Rect(t_start-1, 0, 2, 10);
        BlendOff();
    }

    /* Pause indicator */
    if(paused){
        Col4(0,0,0,0.5f);
        Rect(W/2.0f-60,H/2.0f-30,120,60);
        Col4(1.0f,0.8f,0.2f,0.9f);
        TextC(W/2.0f,H/2.0f+5,"  PAUSED",GLUT_BITMAP_HELVETICA_18);
        Col4(0.7f,0.7f,0.7f,0.7f);
        TextC(W/2.0f,H/2.0f-20,"Press [P] to continue",GLUT_BITMAP_HELVETICA_10);
    }

    BlendOff();
}

/* ══════════════════════════════════════════════════════
   UPDATE SCENE
   ══════════════════════════════════════════════════════ */
static void update_scene(void){
    if(gt<T1)       scene=0;
    else if(gt<T2)  scene=1;
    else if(gt<T3)  scene=2;
    else if(gt<T4)  scene=3;
    else if(gt<T5)  scene=4;
    else if(gt<T6)  scene=5;
    else if(gt<T7)  scene=6;
    else            scene=7;
}

/* ══════════════════════════════════════════════════════
   GLUT CALLBACKS
   ══════════════════════════════════════════════════════ */
static void display(void){
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    update_scene();

    /* ─── CAMERA PAN & SHAKE ─── */
    glPushMatrix();
    /* Subtle handheld camera float */
    glTranslatef(sinf(gt*0.8f)*1.5f, cosf(gt*1.1f)*1.0f, 0);

    /* Shake logic (explosions, sorting impacts, furnace rumble) */
    float shake_x = 0, shake_y = 0;
    if(scene == 4 && sinf(gt*20) > 0.8f) { /* sorting arms */
        shake_x = (frand() - 0.5f) * 1.5f;
        shake_y = (frand() - 0.5f) * 1.5f;
    }
    if(scene == 5 && frand() < 0.2f) { /* furnace combustion */
        shake_x = (frand() - 0.5f) * 1.5f;
        shake_y = (frand() - 0.5f) * 1.5f;
    }
    glTranslatef(shake_x, shake_y, 0);

    switch(scene){
        case 0: draw_title();           break;
        case 1: draw_problem_city();    break;
        case 2: draw_iot_network();     break;
        case 3: draw_collection_scene();break;
        case 4: draw_sorting_facility();break;
        case 5: draw_energy_plant();    break;
        case 6: draw_dashboard();       break;
        case 7: draw_future_city();     break;
    }
    glPopMatrix();

    drawVignette();
    draw_hud();
    glutSwapBuffers();
}

static void timer_cb(int v){
    int now=glutGet(GLUT_ELAPSED_TIME);
    float dt=(now-lastMs)/1000.0f;
    if(dt>0.05f) dt=0.05f;
    lastMs=now;

    if(!paused){
        fps=0.92f*fps+0.08f*(1.0f/(dt+1e-6f));
        gt+=dt;
        if(gt>T8) gt=T8;
        if(!paused) UpdateParticles(dt);
    }

    glutPostRedisplay();
    glutTimerFunc(16,timer_cb,0);
}

static void keyboard_cb(unsigned char key,int x,int y){
    switch(key){
        case 27: case 'q': case 'Q': exit(0);
        case 'p': case 'P': paused=!paused; break;
        case 'r': case 'R':
            gt=0; scene=0; paused=0; nparts=0;
            sbins_init=0; trucks_init=0; sitems_init=0;
            break;
        case '1': gt=T1+0.5f; break;
        case '2': gt=T2+0.5f; break;
        case '3': gt=T3+0.5f; break;
        case '4': gt=T4+0.5f; break;
        case '5': gt=T5+0.5f; break;
        case '6': gt=T6+0.5f; break;
        case '7': gt=T7+0.5f; break;
        case '8': gt=0;        break;
    }
}

static void reshape_cb(int w2,int h2){
    glViewport(0,0,w2,h2);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/* ══════════════════════════════════════════════════════
   MAIN
   ══════════════════════════════════════════════════════ */
int main(int argc,char**argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(W,H);
    glutInitWindowPosition(80,60);
    glutCreateWindow("Urban Waste Management — Smart City 5-Minute Animation");

    glClearColor(0,0,0,1);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape_cb);
    glutKeyboardFunc(keyboard_cb);

    lastMs=glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16,timer_cb,0);

    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║   URBAN WASTE MANAGEMENT — 5-Min Animation      ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  S0  0:00–0:12  Cinematic Title                 ║\n");
    printf("║  S1  0:12–0:55  The Waste Crisis                ║\n");
    printf("║  S2  0:55–1:45  Smart Bins & IoT Network        ║\n");
    printf("║  S3  1:45–2:35  Automated Collection Routes     ║\n");
    printf("║  S4  2:35–3:20  Sorting Facility & Recycling    ║\n");
    printf("║  S5  3:20–4:05  Waste-to-Energy Plant           ║\n");
    printf("║  S6  4:05–4:35  City Dashboard & Analytics      ║\n");
    printf("║  S7  4:35–5:00  Zero-Waste Future               ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  [P] Pause   [R] Restart   [1-8] Jump Scene    ║\n");
    printf("║  [Q/Esc] Quit                                   ║\n");
    printf("╚══════════════════════════════════════════════════╝\n\n");

    glutMainLoop();
    return 0;
}
