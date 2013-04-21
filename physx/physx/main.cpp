//#define _CRTDBG_MAP_ALLOC

#include <iostream>
#include <GL/glut.h>
#include <vector>
#define WIN32
#include <NxPhysics.h>
#include <NxStream.h>
#include <NxCooking.h>

#include "Terrain.h"
#include "ModelOBJ.h"
#include "Stream.h"
#include "ErrorStream.h"
#include "UserAllocator.h"
#include "ObjMesh.h"
#include "MediaPath.h"



using namespace std;

const int	WINDOW_WIDTH=1024, 
WINDOW_HEIGHT=768;

// SDK parameter
ErrorStream			gErrorStream;
UserAllocator*		gAllocator				= NULL;
NxU32				mMeshDirtyFlags;

static NxPhysicsSDK*		gPhysicsSDK		= NULL;
static NxCookingInterface	*gCooking		= NULL;
NxScene*					gScene			= NULL;
NxReal						myTimestep		= 1.0f/120.0f;
NxActor*					groundPlane; 

//wind parameter
NxReal	range		= 20.0f;
float	windDirA	= 1.0;
float	windDirD	= 1.0;
bool	stopwind	= true;

//virtual Capsule
int gInvisible;

//here is for height field
char*				terrianFileName		= "media/scene.obj";
ObjMesh				*terrianObjMesh;
static NxActor*		gHeightField		= NULL;


//terrain
static NxActor*				gTerrain		= NULL;
static NxTriangleMesh*		terrainMesh		= NULL;
static TerrainData*			gTerrainData	= NULL;
//----------------------------------
#define INIT_PARTICLES	10000

struct ParticleSDK
{
	NxVec3	position;
	NxVec3  velocity;
	NxReal	density;
	NxReal  lifetime;
	NxU32	id;
	NxVec3	collisionNormal;
};

//fliud
bool bHardwareFluid = true; // make it "true" if u have PhysX enabled GPU.

#define MAX_PARTICLES 60000//65500 // Fluid particle macros

//Fluid particle globals
NxU32			gNumParticles	= 0;
NxVec3*			gParticles		= 0;
NxVec3*			gParticlesNor	= 0;

//Fluid globals
NxFluid*		fluid			= NULL;
NxFluidEmitter*	fluidEmitter	= 0;
NxActor*		Drain			=NULL;

//tree Model paramter
char*			treeFileName		= "media/tree";
NxSoftBody		*mTreeMesh			= NULL;
ObjMesh			*objMesh;
NxArray<NxVec3>	treeTempVertices;
NxArray<NxU32>	treeTempIndices;


//leaf Model paramter
ModelOBJ*		model;
const float*	vertex;
const int*		index;
int				NbVertex;
int				NbTriangle;
char*			leafFileName		= "media/leaf.obj";
const int		sizeOfLeaves		=50;


NxCloth*		mLeaves[sizeOfLeaves];
NxU32			mNumIndices[sizeOfLeaves]; 
NxU32			mNumVertices[sizeOfLeaves];

//vector<vector<NxVec3>> pos;
//vector<vector<NxVec3>> normal;
//vector<vector<NxU32>> indices;
vector<NxVec3> pos[sizeOfLeaves];
vector<NxVec3> normal[sizeOfLeaves];
vector<NxU32> indices[sizeOfLeaves];

//buffer for store data
NxMeshData receiveBuffers[sizeOfLeaves];
NxMeshData receiveBuffers1;

//GPU enable control
bool mGPU = true;

//for mouse dragging
int oldX=0, oldY=0;
float rX=15, rY=0;
float fps=0;
int startTime=0;
int totalFrames=0;
int state =1 ;
float dist=-40;

//funciton for free vector memory
template <class T>
void FreeAll(vector<T*>* deleteme) {
    while(!deleteme->empty()) {
        delete deleteme->back();
        deleteme->pop_back();
    }

    delete deleteme;
}

//function list
NxActor* CreateCapsule(const NxVec3& pos0, const NxReal height1, const NxReal radius1);
NxActor* CreateFluidDrain();
void DisplayStats();
void SetOrthoForFont();
void ResetPerspectiveProjection();
void RenderSpacedBitmapString(int x, int y,int spacing, void *font, char *string);
void DrawAxes();
void DrawGrid(int GRID_SIZE);
void StepPhysX();
void RenderLeaf(int InPos);
void RenderTree();
void createLeaf(int	InPos,NxVec3 position);
bool createTree();
bool ReadData();
bool InitializePhysX();
void ShutdownPhysX();
void InitGL();
void OnReshape(int nw, int nh);
void OnRender();
void OnShutdown();
void Mouse(int button, int s, int x, int y);
void KeyboardCallback(unsigned char key, int x, int y);
void Motion(int x, int y);
void OnIdle();
void main(int argc, char** argv);


void DrawLine(const NxVec3& p0, const NxVec3& p1, const NxVec3& color, float lineWidth = 1.0)
{
	glDisable(GL_LIGHTING);
	glLineWidth(lineWidth);
	glColor4f(color.x, color.y, color.z, 1.0f);
	NxVec3 av3LineEndpoints[] = {p0, p1};
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(NxVec3), &av3LineEndpoints[0].x);
	glDrawArrays(GL_LINES, 0, 2);
	glDisableClientState(GL_VERTEX_ARRAY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_LIGHTING);
}
void DrawCircle(NxU32 nbSegments, const NxMat34& matrix, const NxVec3& color, const NxF32 radius, const bool semicircle = false)
{
	NxF32 step = NxTwoPiF32/NxF32(nbSegments);
	NxU32 segs = nbSegments;
	if(semicircle)
	{
		segs /= 2;
	}

	for(NxU32 i=0;i<segs;i++)
	{
		NxU32 j=i+1;
		if(j==nbSegments)	j=0;

		NxF32 angle0 = NxF32(i)*step;
		NxF32 angle1 = NxF32(j)*step;

		NxVec3 p0,p1;
		matrix.multiply(NxVec3(radius * sinf(angle0), radius * cosf(angle0), 0.0f), p0);
		matrix.multiply(NxVec3(radius * sinf(angle1), radius * cosf(angle1), 0.0f), p1);

		DrawLine(p0, p1, color);
	}
}

void DrawBox(NxShape* pShape) {
	NxVec3 color;
	color = NxVec3(1.0,0.0,0.0);
	NxMat34 pose = pShape->getGlobalPose();

	NxReal r = pShape->isCapsule()->getRadius();
	NxReal h = pShape->isCapsule()->getHeight();

	NxSegment SG;
	pose.M.getColumn(1, SG.p1);
	SG.p1 *= 0.5f*h;
	SG.p0 = -SG.p1;
	SG.p0 += pose.t;
	SG.p1 += pose.t;

	NxVec3 c0;	pose.M.getColumn(0, c0);
	NxVec3 c1;	pose.M.getColumn(1, c1);
	NxVec3 c2;	pose.M.getColumn(2, c2);
	DrawLine(SG.p0 + c0*r, SG.p1 + c0*r, color);
	DrawLine(SG.p0 - c0*r, SG.p1 - c0*r, color);
	DrawLine(SG.p0 + c2*r, SG.p1 + c2*r, color);
	DrawLine(SG.p0 - c2*r, SG.p1 - c2*r, color);

	pose.M.setColumn(0, -c1);
	pose.M.setColumn(1, -c0);
	pose.M.setColumn(2, c2);
	pose.t = SG.p0;
	DrawCircle(20, pose, color, r, true);	//halfcircle -- flipped

	pose.M.setColumn(0, c1);
	pose.M.setColumn(1, -c0);
	pose.M.setColumn(2, c2);
	pose.t = SG.p1;
	DrawCircle(20, pose, color, r, true);

	pose.M.setColumn(0, -c1);
	pose.M.setColumn(1, c2);
	pose.M.setColumn(2, c0);
	pose.t = SG.p0;
	DrawCircle(20, pose, color, r, true);//halfcircle -- good

	pose.M.setColumn(0, c1);
	pose.M.setColumn(1, c2);
	pose.M.setColumn(2, c0);
	pose.t = SG.p1;
	DrawCircle(20, pose, color, r, true);

	pose.M.setColumn(0, c2);
	pose.M.setColumn(1, c0);
	pose.M.setColumn(2, c1);
	pose.t = SG.p0;
	DrawCircle(20, pose, color, r);	//full circle
	pose.t = SG.p1;
	DrawCircle(20, pose, color, r);
}
void DrawShape(NxShape* shape) 
{ 
	int type = shape->getType();
    switch(type) 
    {          
		case NX_SHAPE_CAPSULE:
			DrawBox(shape);
		break;
    } 
} 

void DrawActor(NxActor* actor) 
{ 
    NxShape* const* shapes = actor->getShapes(); 
    NxU32 nShapes = actor->getNbShapes(); 
    nShapes = actor->getNbShapes(); 
    while (nShapes--) 
    { 
        DrawShape(shapes[nShapes]); 
    } 
} 

void RenderActors() 
{ 
    // Render all the actors in the scene 
    int nbActors = gScene->getNbActors(); 
    NxActor** actors = gScene->getActors(); 
    while (nbActors--) 
    { 
        NxActor* actor = *actors++;
        DrawActor(actor); 
    } 
} 
 


//-------------------------------------------------
NxActor* CreateFluidDrain()
{
	NxBoxShapeDesc DrainDesc;
	NxActorDesc DrainActorDesc;

	DrainDesc.setToDefault();
	DrainDesc.shapeFlags |=NX_SF_FLUID_DRAIN;
	DrainDesc.dimensions.set(20.0f,3.0f,0.01f);
	DrainActorDesc.shapes.pushBack(&DrainDesc);


	NxActor *drainActor=gScene ->createActor(DrainActorDesc);

	

	drainActor->setGlobalPosition(NxVec3(5,12,30));

	return drainActor;
}

NxVec3 RandNormalVec()
{
	NxReal x, y, z;
	NxReal s=0;

	//choose direction, uniformly distributed.

	do
	{
		s++;
		x = 5.0f + NxMath::rand(-17.0f, 17.0f);
		y = 20.0 + NxMath::rand(0.0f, 15.0f);
		z = 15.0f + NxMath::rand(-4.0f, 10.0f);

	} while (s <INIT_PARTICLES);

	NxVec3 dir(x,y,z);


	return dir;
}

NxFluid* CreateFluid()
{

	ParticleSDK*	initParticles = new ParticleSDK[INIT_PARTICLES];
	unsigned initParticlesNum = 0;

	while (initParticlesNum < INIT_PARTICLES)
	{
		NxVec3 pos;

		pos = RandNormalVec();

		ParticleSDK& newParticle = initParticles[initParticlesNum++];
		
		newParticle.position = pos;
		newParticle.velocity = NxVec3(0.0f,0.0f,0.0f);
	}

	NxParticleData particles;

	particles.numParticlesPtr		= &initParticlesNum;
	particles.bufferPos				= &initParticles[0].position.x;
	particles.bufferPosByteStride	= sizeof(ParticleSDK);
	particles.bufferVel				= &initParticles[0].velocity.x;
	particles.bufferVelByteStride	= sizeof(ParticleSDK);
	particles.bufferLife			= &initParticles[0].lifetime;
	particles.bufferLifeByteStride	= sizeof(ParticleSDK);
	

	// Create fluid
	NxFluidDesc fluidDesc;
	//fluidDesc.setToDefault();
	fluidDesc.maxParticles						= 15000; //max number of particles we can simulate
	fluidDesc.kernelRadiusMultiplier			= 2.0f; //distance at which particles can influence each other
	fluidDesc.restParticlesPerMeter				= 1.5f; //number of particles of fluid that exist per meter when the fluid is at rest
	fluidDesc.motionLimitMultiplier				= 3.0f;
	fluidDesc.packetSizeMultiplier				= 8;
	fluidDesc.collisionDistanceMultiplier		= 0.1f;
	fluidDesc.stiffness							= 50.0f; //fluid's resistance to compression
	fluidDesc.viscosity							= 10.0f; //the viscosity of the fluid, describing the level of attraction of the fluid particles
	fluidDesc.restDensity						= 200.0f;
	fluidDesc.damping							= 0.0f; //this damps the velocity of the particles
	fluidDesc.restitutionForStaticShapes		= 0.0f; //coefficient of restitution of the fluid with static shapes
	fluidDesc.restitutionForDynamicShapes		= 0.0f;
	fluidDesc.dynamicFrictionForStaticShapes	= 0.08f; //friction that cause energy loses of fluid as it slides over a static surface.
	fluidDesc.dynamicFrictionForDynamicShapes	= 0.08f;
	//fluidDesc.collisionMethod					= NX_F_STATIC;
	fluidDesc.staticFrictionForStaticShapes		= 0.0f;
	fluidDesc.staticFrictionForDynamicShapes	= 0.0f;
	fluidDesc.numReserveParticles				= 500;
	fluidDesc.simulationMethod					= NX_F_SPH; // NX_F_NO_PARTICLE_INTERACTION; //setting the flag that particles should interact with each other
	//fluidDesc.collisionMethod = NX_F_STATIC;
	fluidDesc.initialParticleData				= particles;
	
	gParticles		= new NxVec3[fluidDesc.maxParticles];
	gParticlesNor	= new NxVec3[fluidDesc.maxParticles];
	fluidDesc.particlesWriteData.bufferPos = &gParticles[0].x;
	fluidDesc.particlesWriteData.bufferPosByteStride = sizeof(NxVec3);
	fluidDesc.particlesWriteData.bufferVel = 0;
	fluidDesc.particlesWriteData.bufferVelByteStride = 0;
	fluidDesc.particlesWriteData.bufferCollisionNormal = &gParticlesNor[0].x;
	fluidDesc.particlesWriteData.bufferCollisionNormalByteStride = sizeof(NxVec3);
	fluidDesc.particlesWriteData.numParticlesPtr = &gNumParticles;

	//fluidDesc.particlesWriteData = particleData;

	fluidDesc.flags |= NX_FF_PRIORITY_MODE ;
	if(mGPU){
		fluidDesc.flags |= NX_FF_HARDWARE;
	}
	
	if(!mGPU)
		fluidDesc.flags &= ~NX_FF_HARDWARE;


	NxFluid* mFluid = gScene->createFluid(fluidDesc);
	//mFluid->setFadeInTime(3);
	/*if(!mFluid)
	{
		fluidDesc.flags &= ~NX_FF_HARDWARE;
		bHardwareFluid = false;
		mFluid = gScene->createFluid(fluidDesc);
	}*/

	return mFluid;
}

NxActor* CreateBox(const NxVec3& pos, const NxVec3& boxDim, const NxReal density)
{
	assert(0 != gScene);

	NxBoxShapeDesc boxDesc;
	boxDesc.dimensions.set(boxDim.x, boxDim.y, boxDim.z);
	boxDesc.localPose.t = NxVec3(0, boxDim.y, 0);

	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&boxDesc);
	actorDesc.globalPose.t = pos;

	NxBodyDesc bodyDesc;
	if (density)
	{
		//actorDesc.body = &bodyDesc;
		actorDesc.density = density;
	}
	else
	{
		actorDesc.body = 0;
	}
	return gScene->createActor(actorDesc);	
}

NxFluidEmitter* CreateFluidEmitter(const NxReal dimX, const NxReal dimY)
{
	fluid = CreateFluid();
	assert(fluid);

	NxQuat q;
	q.fromAngleAxis(-90,NxVec3(1,0,0));
	NxMat34 mat;
	mat.M.fromQuat(q);
	//mat.M.rotZ(1.57);
	mat.t = NxVec3(0,1.2f,0);
	
	NxActor* mBox = CreateBox(NxVec3(0,4,0),NxVec3(1.0,1.0,1.0),2); // Create frame actor
	NxMat33 m, n;
	m.rotX(NxPi/2);
	//n.rotZ(NxPi/6);
	//m = m*n;
	mBox->setGlobalOrientation(m);
	//mBox->setGlobalPosition(NxVec3(4,14,-2));
	mBox->setGlobalPosition(NxVec3(5,40,10));
	//gSelectedActor = mBox;
	
	NxFluidEmitterDesc emitterDesc; // Create emitter
	emitterDesc.setToDefault();
	emitterDesc.frameShape				= mBox->getShapes()[0];
	emitterDesc.dimensionX				= dimX;
	emitterDesc.dimensionY				= dimY;
	emitterDesc.relPose					= mat; //emitter matrix is called the “relPose?bcoz we can attach it to an actor.
	emitterDesc.rate					= 500; //number of fluid particles ejected per second
	emitterDesc.randomAngle				= 0.01f; //angle by which a particle may deviate from the emitter's direction when spawned
	emitterDesc.randomPos				= NxVec3(0.0f,0.0f,0.0f);
	emitterDesc.fluidVelocityMagnitude	= 4.0f; //fluid particles' initial velocity, in meters per second
	emitterDesc.repulsionCoefficient	= 0.9f;
	emitterDesc.maxParticles			= 0; //max number of particles the emitter can emit
	emitterDesc.particleLifetime		= 2.0; //lifetime of the particles, in seconds
	emitterDesc.type					= NX_FE_CONSTANT_FLOW_RATE;
	emitterDesc.shape					= NX_FE_ELLIPSE;
	return fluid->createEmitter(emitterDesc);
}

NxActor* CreateCapsule(const NxVec3& pos0, const NxReal height1, const NxReal radius1,NxReal x, NxReal y, NxReal z)
{
	// Add a single-shape actor to the scene
	NxCapsuleShapeDesc capsuleDesc1;
	capsuleDesc1.height = height1;
	capsuleDesc1.radius = radius1;

	NxMat33 rot0, rot1,rot2;
	rot0.rotX(x);
	rot1.rotY(y);
	rot2.rotZ(z);
	rot0 = rot0*rot1*rot2;
	capsuleDesc1.localPose.M = rot0;

	NxActorDesc actorDesc;

	actorDesc.shapes.pushBack(&capsuleDesc1);
	actorDesc.density = 1.0;
	actorDesc.globalPose.t = pos0;

	return gScene->createActor(actorDesc);	
}

//display GPU state
void DisplayStats()
{
#ifdef WIN32 
	static NxI32 totalHeapIdx = -1;
	static NxI32 deformableHeapIdx = -1;
	static NxI32 utilHeapIdx = -1;


		const NxSceneStats2* stats = gScene->getStats2();
		NxU32 numStats = stats->numStats;
		if (totalHeapIdx == -1)
		{
			// Figure out indices for stats. This should only need to be done once.
			for (NxU32 curIdx = 0; curIdx < stats->numStats; ++curIdx)
			{
				if (0 == strncmp(stats->stats[curIdx].name, "GpuHeapUsageTotal", 17))
					totalHeapIdx = curIdx;
				else if (0 == strncmp(stats->stats[curIdx].name, "GpuHeapUsageDeformable", 17))
					deformableHeapIdx = curIdx;
				else if (0 == strncmp(stats->stats[curIdx].name, "GpuHeapUsageUtils", 17))
					utilHeapIdx = curIdx;
			}
		}

		if ((totalHeapIdx >= 0) && ((NxU32)totalHeapIdx < numStats))
		{
			NxSceneStatistic& stat = stats->stats[totalHeapIdx];
			cout<<"GpuHeapUsageTotal: ="<<stat.curValue  <<",  max = " << stat.maxValue<<"kB\n";	
		}

		if ((deformableHeapIdx >= 0) && ((NxU32)deformableHeapIdx < numStats))
		{
			NxSceneStatistic& stat = stats->stats[deformableHeapIdx];
		cout<<"GpuHeapUsageDeformable: ="<<stat.curValue  <<",  max = " << stat.maxValue<<"kB\n";
		}

		if ((utilHeapIdx >= 0) && ((NxU32)utilHeapIdx < numStats))
		{
			NxSceneStatistic& stat = stats->stats[utilHeapIdx];
			cout<<"GpuHeapUsageUtils: ="<<stat.curValue  <<",  max = " << stat.maxValue<<"kB\n";	
		}
	
#endif
}

void SetOrthoForFont()
{	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
	glScalef(1, -1, 1);
	glTranslatef(0, -WINDOW_HEIGHT, 0);
	glMatrixMode(GL_MODELVIEW);	
	glLoadIdentity();
}

void ResetPerspectiveProjection() 
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

//show FPS
void RenderSpacedBitmapString(
							  int x, 
							  int y,
							  int spacing, 
							  void *font,
							  char *string) 
{
	char *c;
	int x1=x;
	for (c=string; *c != '\0'; c++) {
		glRasterPos2i(x1,y);
		glutBitmapCharacter(font, *c);
		x1 = x1 + glutBitmapWidth(font,*c) + spacing;
	}
}


void DrawAxes()
{	 
	//To prevent the view from disturbed on repaint
	//this push matrix call stores the current matrix state
	//and restores it once we are done with the arrow rendering
	glPushMatrix();
		glColor3f(0,0,1);
		glPushMatrix();
			glTranslatef(0,0, 0.8f);
			glutSolidCone(0.0325,0.2, 4,1);
			//Draw label			
			glTranslatef(0,0.0625,0.225f);
			RenderSpacedBitmapString(0,0,0,GLUT_BITMAP_HELVETICA_10, "Z");
		glPopMatrix();					
		glutSolidCone(0.0225,1, 4,1);

		glColor3f(1,0,0);
		glRotatef(90,0,1,0);	
		glPushMatrix();
			glTranslatef(0,0,0.8f);
			glutSolidCone(0.0325,0.2, 4,1);
			//Draw label
			glTranslatef(0,0.0625,0.225f);
			RenderSpacedBitmapString(0,0,0,GLUT_BITMAP_HELVETICA_10, "X");
		glPopMatrix();					
		glutSolidCone(0.0225,1, 4,1);

		glColor3f(0,1,0);
		glRotatef(90,-1,0,0);	
		glPushMatrix();
			glTranslatef(0,0, 0.8f);
			glutSolidCone(0.0325,0.2, 4,1);
			//Draw label
			glTranslatef(0,0.0625,0.225f);
			RenderSpacedBitmapString(0,0,0,GLUT_BITMAP_HELVETICA_10, "Y");
		glPopMatrix();					
		glutSolidCone(0.0225,1, 4,1);	
	glPopMatrix();
}

void DrawGrid(int GRID_SIZE)
{
	glBegin(GL_LINES);
	glColor3f(0.64f, 0.328f, 0.18f);
	for(int i=-GRID_SIZE;i<=GRID_SIZE;i++)
	{
		glVertex3f((float)i,0,(float)-GRID_SIZE);
		glVertex3f((float)i,0,(float)GRID_SIZE);

		glVertex3f((float)-GRID_SIZE,0,(float)i);
		glVertex3f((float)GRID_SIZE,0,(float)i);
	}
	glEnd();
}

//run physx 
void StepPhysX() 
{ 
	gScene->simulate(myTimestep);        
	gScene->flushStream(); 
	gScene->fetchResults(NX_RIGID_BODY_FINISHED, true); 
	//...perform useful work here using previous frame's state data
	// add wind acceleration to the leaves and tree
	if(stopwind)
	{
		for( int InPos = 0; InPos<sizeOfLeaves;InPos++)
		{
			mLeaves[InPos]->setWindAcceleration(mTreeMesh->getExternalAcceleration()+NxVec3( NxMath::rand(5, 25),NxMath::rand(9.8,10 ),NxMath::rand(0, 4)));
		}

		if(mTreeMesh)
		{
			//mTreeMesh->wakeUp();
			mTreeMesh->setExternalAcceleration(NxVec3( NxMath::rand(0.0f, 1.0f), NxMath::rand(-1.0f, 2.0f), NxMath::rand(-1.0f, 2.0f)));
	
		}

	}else{

		for( int InPos = 0; InPos<sizeOfLeaves; InPos++)
		{
			mLeaves[InPos]->setWindAcceleration(NxVec3(0.0,0.0,0.0));
			
		}
		
		if(mTreeMesh)
		{
			mTreeMesh->setExternalAcceleration(NxVec3(0.0f,0.0f,0.0f));	
		}
	} 
	
	for( int InPos = 0; InPos<sizeOfLeaves;InPos++)
	{
		NxVec3 position = mLeaves[InPos]->getPosition(0);
		if(!(-100.0 < position.x && position.x < 90.0 &&
				0.0 < position.y && position.y < 120.0 &&
				0.0 < position.z && position.z < 150.0)) 
		{
			float x = rand() % 30;
			float y = rand() % 70 ;
			float z = rand() % 50;
			gScene->releaseCloth(*mLeaves[InPos]);
			createLeaf(InPos,NxVec3(-90.0+x,20.0+y,30.0+z));
		}

	}

} 

bool ReadData()
{
	char tetFileName[256], objFileName[256], s[256];
	static const NxU16 MESH_STRING_LEN = 256;
	char buf[MESH_STRING_LEN];
	int i0,i1,i2,i3;
	NxVec3 v;

	// terrian data
	terrianObjMesh = new ObjMesh();
	terrianObjMesh->loadFromObjFile(FindMediaFile(terrianFileName,s));
	cout<<"load terrian"<<"\n";

	//tree data
	sprintf(tetFileName, "%s.tet", treeFileName);
	sprintf(objFileName, "%s.obj", treeFileName);

	objMesh = new ObjMesh();
	objMesh->loadFromObjFile(FindMediaFile(objFileName, s));
	FILE* f = fopen(tetFileName, "r");
	if (!f)
		return false;
	
	while (!feof(f)) {
		if (fgets(buf,MESH_STRING_LEN, f) == NULL) break;

		if (strncmp(buf, "v ", 2) == 0) {	// vertex
			sscanf(buf, "v %f %f %f", &v.x, &v.y, &v.z);
			treeTempVertices.push_back(v);
		}
		else if (strncmp(buf, "t ", 2) == 0) {	// tetra
			sscanf(buf, "t %i %i %i %i", &i0,&i1,&i2,&i3);
			treeTempIndices.push_back(i0);
			treeTempIndices.push_back(i1);
			treeTempIndices.push_back(i2);
			treeTempIndices.push_back(i3);
		}
	}

	if(treeTempVertices.size() == 0) 
		return false;

	cout<<"Load tree model finished\n";
	
	//leaf data
	model = new ModelOBJ();
	model->import(leafFileName);
	vertex = model->getVertexCoordBuffer();
	index = model->getIndexBuffer();
	NbVertex = model->getNbVertex();
	NbTriangle = model->getNbTriangles();

	cout<<"Load leaf model finished"<<"\n";
	
	return true;
}

void RenderFluid() //just draw boxes for the particles
 {
	 glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
	 for (NxU32 p=0; p<gNumParticles; p++)
	 {
		 NxVec3& particle = gParticles[p];

		 glPushMatrix();
		 glPointSize(5.0);
		 glTranslatef(particle.x,particle.y,particle.z);
		 //renderSolidCube(0.1);   
		// glutSolidSphere(1.0,8,5);
		glBegin(GL_POINTS);
		glVertex2f(0.0,0.0);
		glEnd();
		 glPopMatrix();
	 }
	 //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void RenderLeaf(int InPos) 
{
	glColor3f(0.184, 0.556, 0.348);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(NxVec3),&(pos[InPos][0].x));
	glNormalPointer(GL_FLOAT, sizeof(NxVec3), &(normal[InPos][0].x));

	glDrawElements(GL_TRIANGLES,indices[InPos].size(), GL_UNSIGNED_INT,&indices[InPos][0]);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void RenderTerrian()
{
	terrianObjMesh->draw(0x8F4B2E, false, false);
}

void RenderTree()
{
	// tree deformation simulation
	objMesh->simulateMesh(receiveBuffers1);
	objMesh->draw(0x2E8B57, false, false);
}

static void CreatePhysicsTerrain()
{
    if(gTerrain != NULL)
	{
		gScene->releaseActor(*gTerrain);
		gTerrain = NULL;
	}
	
	NxVec3* verts=new NxVec3[terrianObjMesh->getNumVertices()];
	NxU32* faces=new NxU32[terrianObjMesh->getNumTriangles()*3];

	for (NxU32 i=0; i<terrianObjMesh->getNumVertices();i++)
	{
		NxVec3 c = terrianObjMesh->getVertex(i);
		verts[i].x = c.x;
		verts[i].y = c.y;
		verts[i].z = c.z;
	}


	for (NxU32 j=0; j<terrianObjMesh->getNumTriangles();j++)
	{
		faces[0+3*j]=terrianObjMesh->getTriangle(j).vertexNr[0];
		faces[1+3*j]=terrianObjMesh->getTriangle(j).vertexNr[1];
		faces[2+3*j]=terrianObjMesh->getTriangle(j).vertexNr[2];
	}
	NxVec3*		normals;
	normals = new NxVec3[terrianObjMesh->getNumVertices()];
	NxBuildSmoothNormals(terrianObjMesh->getNumTriangles(), terrianObjMesh->getNumVertices(), verts, faces, NULL, normals, true);

	NxTriangleMeshDesc terrainDesc;
	terrainDesc.numVertices                 = terrianObjMesh->getNumVertices();
	terrainDesc.numTriangles				= terrianObjMesh->getNumTriangles();
	terrainDesc.pointStrideBytes			= sizeof(NxVec3);
	terrainDesc.triangleStrideBytes			= 3*sizeof(NxU32);

	terrainDesc.points						= verts;
	terrainDesc.triangles					= faces;
	terrainDesc.flags						= 0;

	terrainDesc.heightFieldVerticalAxis		= NX_Y;
	terrainDesc.heightFieldVerticalExtent	= 1000.0f;

	gCooking->NxCloseCooking();
	bool status;
	if(!gCooking->NxInitCooking(gAllocator,&gErrorStream)){
		cout<<"\nError: Unable to initialize the cooking library, exiting the sample.\n\n";
	}
	MemoryWriteBuffer buf;
	status = gCooking->NxCookTriangleMesh(terrainDesc, buf);
	if (!status) {
		printf("Unable to cook a triangle mesh.");
	//	exit(1);
	}
	MemoryReadBuffer readBuffer(buf.data);
	terrainMesh = gPhysicsSDK->createTriangleMesh(readBuffer);

	//
	// Please note about the created Triangle Mesh, user needs to release it when no one uses it to save memory. It can be detected
	// by API "meshData->getReferenceCount() == 0". And, the release API is "gPhysicsSDK->releaseTriangleMesh(*meshData);"
	//

	NxTriangleMeshShapeDesc terrainShapeDesc;
	terrainShapeDesc.meshData				= terrainMesh;
	terrainShapeDesc.shapeFlags				= NX_SF_FEATURE_INDICES;
	//terrainShapeDesc.flags |= NxShapeFlag::NX_SF_FLUID_TWOWAY;

	NxActorDesc ActorDesc;
	ActorDesc.shapes.pushBack(&terrainShapeDesc);
	ActorDesc.globalPose.t = NxVec3(0.0f,0.0f,0.0f);
	gTerrain = gScene->createActor(ActorDesc);
	//gTerrain->userData = (void*)0;
	
	gCooking->NxCloseCooking();
	delete[] verts;
	delete[] faces;

	if(terrainMesh->getReferenceCount() == 0)
		gPhysicsSDK->releaseTriangleMesh(*terrainMesh);
}

void createLeaf(int	InPos,NxVec3 position)
{
	NxClothDesc desc; 
	desc.globalPose.t = position;
	desc.thickness						= 0.5f;
	desc.bendingStiffness				= 1.0;
	desc.stretchingStiffness			= 1.0;
	desc.dampingCoefficient				= 0.9f;
	desc.solverIterations				= 3;	
	desc.friction						= 0.1;
	desc.density						= 1000.0;

	desc.flags	|=	NX_CLF_GRAVITY;
	//desc.flags	|=	NX_CLF_UNTANGLING;
	desc.flags	|=	NX_CLF_SELFCOLLISION;
	desc.flags	|=	NX_CLF_DAMPING;
	desc.flags  |=	NX_CLF_COMDAMPING;
	desc.flags	|=	NX_CLF_COLLISION_TWOWAY;
	if(mGPU)
		desc.flags	|=	NX_CLF_HARDWARE; 
	if(!mGPU)
		desc.flags	&=	~NX_CLF_HARDWARE;

	desc.flags	|=	NX_CLF_VISUALIZATION;
	
	NxClothMeshDesc meshDesc;  
	meshDesc.numVertices				= NbVertex;        
	meshDesc.numTriangles				= NbTriangle;    
	meshDesc.pointStrideBytes			= sizeof(NxVec3);  
	meshDesc.triangleStrideBytes		= 3*sizeof(NxU32);
	meshDesc.vertexMassStrideBytes		= sizeof(NxReal);
	meshDesc.vertexFlagStrideBytes		= sizeof(NxU32);
	meshDesc.points						= (NxVec3*)malloc(sizeof(NxVec3)*meshDesc.numVertices);    
	meshDesc.triangles					= (NxU32*)malloc(sizeof(NxU32)*meshDesc.numTriangles*3);
	meshDesc.vertexMasses				= 0;
	meshDesc.vertexFlags				= 0;
	meshDesc.flags						= 0;
	meshDesc.flags						|= NX_CLOTH_MESH_WELD_VERTICES;
	meshDesc.weldingDistance			= 0.5f;

	//Fill the geometry
	int i;    
	if(pos[InPos].size() == 0){
	pos[InPos].resize(meshDesc.numVertices);
 	normal[InPos].resize(meshDesc.numVertices);
	indices[InPos].resize(meshDesc.numTriangles*3);
	}

	NxVec3 *p = (NxVec3*)meshDesc.points; 
	for(i = 0; i < NbVertex; i++){
		p->x = vertex[3*i];
		p->y = vertex[3*i+1];
		p->z = vertex[3*i+2];
		p++;
	}
	
	//Fill the topology
	NxU32 *id = (NxU32*)meshDesc.triangles;  
	for (i = 0; i < NbTriangle*3 ; i++){
		NxU32 c	= index[i];
		id[i] = c;
	}
	
	//Start cooking 
	//Make sure everything is fine so far
	if(!(meshDesc.isValid()))
	   cerr<<"Mesh invalid."<<endl;

	if(!gCooking->NxInitCooking(gAllocator,&gErrorStream)){
		cout<<"\nError: Unable to initialize the cooking library, exiting the sample.\n\n";
	}

	MemoryWriteBuffer wb;
		if (!gCooking->NxCookClothMesh(meshDesc, wb))         
			return;
	MemoryReadBuffer rb(wb.data);
	NxClothMesh *mClothMesh = gScene->getPhysicsSDK().createClothMesh(rb);

	NxVec3* y= (NxVec3*)meshDesc.points;
	NxU32* t = (NxU32*)meshDesc.triangles;
	NxReal* m = (NxReal*)meshDesc.vertexMasses;
	NxU32* f = (NxU32*)meshDesc.vertexFlags;
	free(y);
	free(t);
	free(m);
	free(f);
	gCooking->NxCloseCooking();

	//Fill in receive buffers...
	receiveBuffers[InPos].setToDefault();
   
	receiveBuffers[InPos].verticesPosBegin			= &(pos[InPos][0].x);
	receiveBuffers[InPos].verticesNormalBegin		= &(normal[InPos][0].x);
	receiveBuffers[InPos].verticesPosByteStride		= sizeof(NxVec3);
	receiveBuffers[InPos].verticesNormalByteStride	= sizeof(NxVec3);
	receiveBuffers[InPos].maxVertices				= meshDesc.numVertices;
	receiveBuffers[InPos].numVerticesPtr			= &mNumVertices[InPos];

	receiveBuffers[InPos].indicesBegin				= &indices[InPos][0];
	receiveBuffers[InPos].indicesByteStride			= sizeof(NxU32);
	receiveBuffers[InPos].maxIndices				= meshDesc.numTriangles*3;
	receiveBuffers[InPos].numIndicesPtr				= &mNumIndices[InPos];

	receiveBuffers[InPos].dirtyBufferFlagsPtr		= &mMeshDirtyFlags;

	desc.clothMesh = mClothMesh;
	desc.meshData  = receiveBuffers[InPos];
	if(!desc.isValid())
		cout<<"wrong";
	mLeaves[InPos]		= gScene->createCloth(desc);
	mLeaves[InPos]->wakeUp();
	//mLeaves[InPos]->setSleepLinearVelocity(0.0);
	mMeshDirtyFlags		= 0;
	mNumVertices[InPos] = 0;
	mNumIndices[InPos]	= 0;
	
}

void createTreeBodyTopo()
{
	NxActor *caps	= CreateCapsule(NxVec3(-77.5f, 4.7f, 52.5f), 8.0f,1.0f,0.3,0.1,-0.1);
	caps->userData	= (void*)&gInvisible;
	NxActor *caps0	= CreateCapsule(NxVec3(-76.0f, 17.0f, 55.0f), 14.0f,1.0f,0.14,0.1,-0.1);
	caps0->userData	= (void*)&gInvisible;
	NxActor *caps1	= CreateCapsule(NxVec3(-76.5f, 40.0f, 62.0f), 30.0f,1.0f,0.4,0.1,0.1);
	caps1->userData = (void*)&gInvisible;
	NxActor *caps8	= CreateCapsule(NxVec3(-76.5f, 71.0f, 72.5f), 30.0f,1.0f,0.3,0.1,-0.1);
	caps8->userData = (void*)&gInvisible;
	NxActor *caps02	= CreateCapsule(NxVec3(-65.0f, 36.0f, 56.5f), 26.0f,1.0f,0.14,0.05,-0.6);
	caps02->userData = (void*)&gInvisible;
	NxActor *caps11	= CreateCapsule(NxVec3(-74.5f, 35.0f, 72.5f), 25.0f,1.0f,1.1,-0.2,-0.1);
	caps11->userData = (void*)&gInvisible;
	NxActor *caps12	= CreateCapsule(NxVec3(-70.0f, 45.0f, 71.5f), 17.0f,1.0f,1.1,0.0,-0.65);
	caps11->userData = (void*)&gInvisible;
	NxActor *caps01	= CreateCapsule(NxVec3(-82.0f, 22.5f, 62.5f), 17.0f,1.0f,1.1, 0.0, 0.6);
	caps11->userData = (void*)&gInvisible;
	NxActor *caps81	= CreateCapsule(NxVec3(-80.0f, 60.0f, 77.5f), 17.0f,1.0f,1.1,0.0, 0.35);
	caps81->userData = (void*)&gInvisible;


	NxActor *caps2	= CreateCapsule(NxVec3(-81.5f, 12.0f, 50.0f), 14.0f,1.0f,-0.05,0.1,0.1);
	caps2->userData = (void*)&gInvisible;
	NxActor *caps3	= CreateCapsule(NxVec3(-85.5f, 32.0f, 51.0f), 20.0f,1.0f,0.17,0.1,0.2);
	caps3->userData = (void*)&gInvisible;
	NxActor *caps4	= CreateCapsule(NxVec3(-88.5f, 50.0f, 52.7f), 10.0f,1.0f,0.05,0.1,0.1);
	caps4->userData = (void*)&gInvisible;
	NxActor *caps9	= CreateCapsule(NxVec3(-96.0f, 66.0f, 52.7f), 20.0f,1.0f,0.05,-0.2,0.6);
	caps9->userData = (void*)&gInvisible;
	NxActor *caps21	= CreateCapsule(NxVec3(-96.0f, 29.0f, 49.0f), 20.0f,1.0f, 0.6, -0.3, 1.2);
	caps21->userData = (void*)&gInvisible;
	NxActor *caps22	= CreateCapsule(NxVec3(-92.0f, 48.0f, 42.0f), 20.0f,1.0f, 2.0, 0.0, -0.35); 
	caps22->userData = (void*)&gInvisible;

	NxActor *caps5	= CreateCapsule(NxVec3(-78.5f, 22.0f, 47.5f), 27.0f,1.0f,-0.15,0.1,0.0);
	caps5->userData = (void*)&gInvisible;
	NxActor *caps6	= CreateCapsule(NxVec3(-76.0f, 42.0f, 44.5f), 8.5f,1.0f,-0.15,0.075,-0.5);
	caps6->userData = (void*)&gInvisible;
	NxActor *caps7	= CreateCapsule(NxVec3(-73.0f, 53.5f, 43.0f), 10.0f,1.0f,-0.15,0.075,-0.0);
	caps7->userData = (void*)&gInvisible;
	NxActor *caps10	= CreateCapsule(NxVec3(-74.0f, 68.0f, 42.0f), 16.0f,1.0f,-0.1,0.075,0.25);
	caps10->userData = (void*)&gInvisible;
	NxActor *caps51	= CreateCapsule(NxVec3(-78.5f, 27.0f, 39.5f), 25.0f,1.0f,-0.6,-0.2, 0.0);//
	caps11->userData = (void*)&gInvisible;
	NxActor *caps61	= CreateCapsule(NxVec3(-74.5f, 48.0f, 34.5f), 18.0f,1.0f,-1.1,-0.2, -0.2);//
	caps61->userData = (void*)&gInvisible;
	
}

bool createTree()
{
	NxSoftBodyDesc softBodyDesc;
	softBodyDesc.particleRadius					= 1.0f;
	softBodyDesc.volumeStiffness				= 0.9f;
	softBodyDesc.stretchingStiffness			= 1.0f;
	softBodyDesc.dampingCoefficient				= 0.8;
	softBodyDesc.friction						= 1.0f;
	softBodyDesc.collisionResponseCoefficient	= 0.9f;
	softBodyDesc.solverIterations				= 1;
	softBodyDesc.flags	|= NX_SBF_COLLISION_TWOWAY;
	//softBodyDesc.flags	|= NX_SBF_STATIC; 
	if(mGPU)
		softBodyDesc.flags |= NX_SBF_HARDWARE;
	if(!mGPU)
		softBodyDesc.flags &= ~NX_SBF_HARDWARE;

	softBodyDesc.flags	|= NX_SBF_VOLUME_CONSERVATION;
 

	NxSoftBodyMeshDesc meshDesc;
	meshDesc.numVertices				= treeTempVertices.size();
	meshDesc.numTetrahedra				= treeTempIndices.size() / 4;
	meshDesc.vertexStrideBytes			= sizeof(NxVec3);
	meshDesc.tetrahedronStrideBytes		= 4*sizeof(NxU32);
	meshDesc.vertexMassStrideBytes		= sizeof(NxReal);
	meshDesc.vertexFlagStrideBytes		= sizeof(NxU32);
	meshDesc.vertices					= (NxVec3*)malloc(sizeof(NxVec3)*meshDesc.numVertices);
	meshDesc.tetrahedra					= (NxU32*)malloc(sizeof(NxU32)*meshDesc.numTetrahedra*4);
	meshDesc.vertexMasses				= 0;
	meshDesc.vertexFlags				= 0;
	meshDesc.flags						= 0;

	int i;
	NxVec3 *vSrc = (NxVec3*)treeTempVertices.begin();
	NxVec3 *vDest = (NxVec3*)meshDesc.vertices;
	for (NxU32 i = 0; i < meshDesc.numVertices; i++, vDest++, vSrc++) 
		*vDest = (*vSrc);
	memcpy((NxU32*)meshDesc.tetrahedra, treeTempIndices.begin(), sizeof(NxU32)*meshDesc.numTetrahedra*4);

	//Start cooking 
	//Make sure everything is fine so far
	if(!(meshDesc.isValid()))
	   cerr<<"Mesh invalid."<<endl;

	if(!gCooking->NxInitCooking(gAllocator,&gErrorStream)){
		cout<<"\nError: Unable to initialize the cooking library, exiting the sample.\n\n";
	}

	MemoryWriteBuffer wb;
		if (!gCooking->NxCookSoftBodyMesh(meshDesc, wb))         
			return false;
	MemoryReadBuffer rb(wb.data);
	NxSoftBodyMesh* mTreeMeshDes = gScene->getPhysicsSDK().createSoftBodyMesh(rb);

	objMesh->buildTetraLinks((NxVec3 *)meshDesc.vertices, (NxU32 *)meshDesc.tetrahedra, meshDesc.numTetrahedra);
	
	NxVec3* y= (NxVec3*)meshDesc.vertices;
	NxU32* t = (NxU32*)meshDesc.tetrahedra;
	NxReal* m = (NxReal*)meshDesc.vertexMasses;
	NxU32* z = (NxU32*)meshDesc.vertexFlags;
	free(y);
	free(t);
	free(m);
	free(z);
	gCooking->NxCloseCooking();

	receiveBuffers1.verticesPosBegin = (NxVec3*)malloc(sizeof(NxVec3)*meshDesc.numVertices);		
	receiveBuffers1.verticesPosByteStride = sizeof(NxVec3);
	receiveBuffers1.maxVertices =  meshDesc.numVertices;
	receiveBuffers1.numVerticesPtr = (NxU32*)malloc(sizeof(NxU32));
	// the number of tetrahedra is constant, even if the softbody is torn
	NxU32 maxIndices = 4*meshDesc.numTetrahedra;
	receiveBuffers1.indicesBegin = (NxU32*)malloc(sizeof(NxU32)*maxIndices);
	receiveBuffers1.indicesByteStride = sizeof(NxU32);
	receiveBuffers1.maxIndices = maxIndices;
	receiveBuffers1.numIndicesPtr = (NxU32*)malloc(sizeof(NxU32));
	// init the buffers in case we want to draw the mesh 
	// before the SDK as filled in the correct values
	*receiveBuffers1.numVerticesPtr = 0;
	*receiveBuffers1.numIndicesPtr = 0;
	

	softBodyDesc.softBodyMesh = mTreeMeshDes;
	softBodyDesc.meshData = receiveBuffers1;
	softBodyDesc.globalPose.t =NxVec3(-80.0,2.7,50.0);
	mTreeMesh = gScene->createSoftBody(softBodyDesc);
	mTreeMesh->wakeUp();

	//create 
	createTreeBodyTopo();

	mTreeMesh->attachToCollidingShapes(0);
	//FreeAll(treeTempVertices);
	//FreeAll(treeTempIndices);
	
	return true;
}

bool InitializePhysX()
{
	if (!gAllocator)
		gAllocator = new UserAllocator;
	// Initialize PhysicsSDK
	NxPhysicsSDKDesc SDKDesc;
	if(mGPU)
	{
		SDKDesc.flags &= ~NX_SDKF_NO_HARDWARE;
		SDKDesc.gpuHeapSize		= 512;
		SDKDesc.meshCacheSize	= 256;
		
	}
		SDKDesc.flags |= NX_SF_SIMULATE_SEPARATE_THREAD; // multi-thread
		SDKDesc.flags |= NX_SDKF_PER_SCENE_BATCHING;//enable GPU resize
	
	NxSDKCreateError errorCode = NXCE_NO_ERROR;
	gPhysicsSDK = NxCreatePhysicsSDK(NX_PHYSICS_SDK_VERSION,gAllocator,NULL,SDKDesc,&errorCode);
	if(gPhysicsSDK == NULL) {
		cerr<<"Error creating PhysX device."<<endl;
		cerr<<"Exiting..."<<endl;
		exit(1);
	}
	


	gCooking = NxGetCookingLib(NX_PHYSICS_SDK_VERSION);

	//Set the debug visualization parameters 
	gPhysicsSDK->getFoundationSDK().getRemoteDebugger()->connect ("localhost", 5425);

    // Set scale dependent parameters 
	gPhysicsSDK->setParameter(NX_SKIN_WIDTH, 0.1f);
	//gPhysicsSDK->setParameter(NX_VISUALIZATION_SCALE, 0.02f);

	
	NxHWVersion hwCheck = gPhysicsSDK->getHWVersion();
	if (hwCheck == NX_HW_VERSION_NONE) 
	{
		cout<<"\nWarning: Turn off GPU(Physx card), Scene will be simulated in software.\n\n";
	}else{
		cout<<"\nWarning: Turn on GPU(Physx card), Scene will be simulated in hardware.\n\n";
	}
	
	//Create the scene
	NxSceneDesc sceneDesc;
	sceneDesc.gravity = NxVec3(0.0f, -9.8f, 0.0f);
	gScene = gPhysicsSDK->createScene(sceneDesc);
	if(gScene == NULL) 
	{
		printf("\nError: Unable to create a PhysX scene, exiting the sample.\n\n");
		return false;
	}
	
	//ReadData();

	/*NxPlaneShapeDesc planeDesc;
	NxActorDesc actorDesc_Plane, actorDesc_Box;

	actorDesc_Plane.shapes.pushBack(&planeDesc);
	gScene->createActor(actorDesc_Plane);  */
	
	//gScene->setTiming(myTimestep / 4.0f);
	
	//3)load tree data
	CreatePhysicsTerrain();
	createTree();
	
	
	//4)create leaf
	
	
	for (int i = 0;i<sizeOfLeaves ;i++)
	{
		float x = rand() % 30;
		float y = rand() % 70 ;
		float z = rand() % 50;
		createLeaf(i,NxVec3(-90.0+x,20.0+y,30.0+z));
	}
	
	//fluid
	fluidEmitter = CreateFluidEmitter(13, 1.0);
	CreateFluidDrain();

	return true;
}

void InitGL() 
{ 
	glClearColor(0.54,0.825,0.94,1.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	
	GLfloat ambient[4]={0.25f,0.25f,0.25f,0.25f};
	GLfloat diffuse[4]={1,1,1,1};
	GLfloat mat_diffuse[4]={1.0f, 1.0f,1.0f};

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);

	
	glDisable(GL_LIGHTING);

	ReadData();
}

void OnReshape(int nw, int nh)
{
	glViewport(0,0,nw, nh);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80, (GLfloat)nw / (GLfloat)nh, 0.1f, 1000.0f);
	glMatrixMode(GL_MODELVIEW);
}

char buffer[256];
void OnRender()
{
	//Calculate fps
	totalFrames++;
	int current = glutGet(GLUT_ELAPSED_TIME);
	if((current-startTime)>1000)
	{		
		float elapsedTime = float(current-startTime);
		fps = ((totalFrames * 1000.0f)/ elapsedTime) ;
		startTime = current;
		totalFrames=0;
	}

	sprintf_s(buffer, "PhysX Demo -- FPS: %3.2f",fps);
	//Show the fps
	glutSetWindowTitle(buffer);

	//Update PhysX	
    if (gScene) 
    { 
        StepPhysX(); 
    } 


	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	glLoadIdentity();
	glTranslatef(0,0,dist);
	glRotatef(rX,1,0,0);
	glRotatef(rY,0,1,0);
	
	//Draw the grid and axes
	glScalef(0.2,0.2,0.2);
	DrawAxes();	
	
	glEnable(GL_LIGHTING);
	RenderTerrian();
	glDisable(GL_LIGHTING);
	

	RenderFluid();
	if(mTreeMesh)
		RenderTree();
	for(int InPos = 0; InPos<sizeOfLeaves;InPos++){
		RenderLeaf(InPos);
	}
	
	RenderActors(); 
	

	SetOrthoForFont();		
	ResetPerspectiveProjection();
	glutSwapBuffers();
}

void ShutdownPhysX() 
{
	NxVec3* vp;
	NxU32* up;
	int i;
	
	vp = (NxVec3*)receiveBuffers1.verticesPosBegin; free(vp);
	up = (NxU32*)receiveBuffers1.numVerticesPtr; free(up);

	up = (NxU32*)receiveBuffers1.indicesBegin; free(up);
	up = (NxU32*)receiveBuffers1.numIndicesPtr; free(up);

	for( i=0; i < gScene->getNbActors();i++)
		gScene->releaseActor(*gScene->getActors()[i]);

	for(i=0; i < gScene->getNbCloths();i++)
		gScene->releaseCloth(*gScene->getCloths()[i]);

	for(i=0; i < gScene->getNbSoftBodies();i++)
		gScene->releaseSoftBody(*gScene->getSoftBodies()[i]);

	for(i=0; i < gScene->getNbFluids();i++)
		gScene->releaseFluid(*gScene->getFluids()[i]);


	gPhysicsSDK->releaseScene(*gScene);
	NxReleasePhysicsSDK(gPhysicsSDK);


	if(gAllocator){
		gAllocator = NULL;
	}
		
}

void OnShutdown()
{
	ShutdownPhysX();
	delete objMesh;	
	free(model);
	delete gAllocator;	
}

void Mouse(int button, int s, int x, int y)
{
	if (s == GLUT_DOWN) 
	{
		oldX = x; 
		oldY = y; 
	}	
	
	if(button == GLUT_MIDDLE_BUTTON)
		state = 0;
	else
		state = 1;
}

void KeyboardCallback(unsigned char key, int x, int y)
{
	switch(key)
	{
	case 'r':case 'R':
		for( int InPos = 0; InPos<sizeOfLeaves;InPos++)
		{
			float x = rand() % 30;
			float y = rand() % 70;
			float z = rand() % 50;
			gScene->releaseCloth(*mLeaves[InPos]);
			createLeaf(InPos,NxVec3(-90.0+x,20.0+y,30.0+z));
		}
		glutPostRedisplay();
		break;

	case 'd':case 'D':
		windDirD = -windDirD;
		glutPostRedisplay();
		break;

	case 's':case 'S':
		if(stopwind){
			stopwind= false;
			glutPostRedisplay();
		}else{
			stopwind= true;
			glutPostRedisplay();
		}
		break;

	case 'l':case 'L':
		DisplayStats();
		break;

	case 'g':case 'G':
		if(mGPU)
			mGPU = false;
		else
			mGPU = true;
		ShutdownPhysX();
		InitializePhysX();
		break;

	case 'n':case 'N':
		cout<<fluidEmitter->getNbParticlesEmitted()<<"\n";
		cout<<gScene->getNbCloths()<<"\n";
		break;

	}	
}

void Motion(int x, int y)
{
	if (state == 0)
		dist *= (1 + (y - oldY)/60.0f); 
	else
	{
		rY += (x - oldX)/5.0f; 
		rX += (y - oldY)/5.0f; 
	} 
	oldX = x; 
	oldY = y; 
	 
	glutPostRedisplay(); 
}

void OnIdle() 
{
	glutPostRedisplay();
}

void main(int argc, char** argv) 
{
	atexit(OnShutdown);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA|GLUT_DEPTH);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("PhysX Demo --");

	glutDisplayFunc(OnRender);
	glutIdleFunc(OnIdle);
	glutReshapeFunc(OnReshape);

	glutKeyboardFunc(KeyboardCallback);
	glutMouseFunc(Mouse);
	glutMotionFunc(Motion);
	InitGL();

	if(InitializePhysX())
	{
		glutMainLoop();
	}		
}
