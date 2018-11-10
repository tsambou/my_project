#include <Bela.h>
#include "gen_exported.h"
#include <Scope.h>

CommonState* gState = NULL;
int nFakeChannels = 2;
Scope scope;
int gCount(0);

void Bela_userSettings(BelaInitSettings *settings)
{
	settings->uniformSampleRate = 1;
	settings->interleave = 0;
	settings->analogOutputsPersist = 0;
}

bool setup(BelaContext *context, void *userData)
{
	if(context->flags & BELA_FLAG_INTERLEAVED)
	{
		fprintf(stderr, "You need a non-interleaved buffer. Found: %#x\n", context->flags);
		return 0;
	}
	if(context->analogSampleRate != context->audioSampleRate)
	{
		fprintf(stderr, "You need the analog and audio channels to have the same sampling rate\n");
		return 0;
	}
	gState = (CommonState*)gen_exported::create(context->audioSampleRate, context->audioFrames);
	scope.setup(nFakeChannels, context->audioSampleRate);
	return true;
}

void render(BelaContext *context, void *userData)
{
	int nFrames = context->audioFrames;

	int nInChannels = context->audioInChannels + context->analogInChannels;
	// taking pointers to the input channels
	t_sample* ip[nInChannels];
	for (unsigned int i = 0; i < context->audioInChannels; i++) {
		ip[i] = (t_sample*)context->audioIn + (i * nFrames);
	}
	for (unsigned int i = 0; i < context->audioInChannels; i++) {
		ip[i + context->audioInChannels] = (t_sample*)context->analogIn + (i * nFrames);
	}

	t_sample fakeChannels[nFakeChannels][context->audioFrames];
	int nOutChannels = context->audioOutChannels + context->analogOutChannels + nFakeChannels;
	// taking pointers to the output channels
	t_sample* op[nOutChannels];
	for (unsigned int i = 0; i < context->audioOutChannels; i++) {
		op[i] = (t_sample*)context->audioOut + (i * nFrames);
	}
	for (unsigned int i = 0; i < context->analogOutChannels; i++) {
		op[i + context->audioOutChannels] = (t_sample*)context->analogOut + (i * nFrames);
	}
	int firstFakeChannel = context->analogOutChannels + context->audioOutChannels;
	for (unsigned int i = 0; i < nFakeChannels; i++) {
		op[i + firstFakeChannel] = (t_sample*)fakeChannels[i];
	}

	gen_exported::perform(gState, ip, nInChannels, op, nOutChannels, nFrames);

	// go through the fake channels and scope them
	for(unsigned int n = 0; n < context->audioFrames; ++n)
	{
		t_sample logArray[nFakeChannels];
		for(unsigned int i = 0; i < nFakeChannels; ++i)
		{
			logArray[i] = fakeChannels[i][n];
			
			if (gCount % 500 == 0) {
    			rt_printf("the value of gVar is %f\n",logArray[1]);
			}
			
			//audioWrite(context, n, 0, 0.05*logArray[1]);
			
			//audioWrite(context, n, i, 0.05*fakeChannels[1][n]);
			
		}
		scope.log(logArray);
	}
	

    gCount++ ;
}

void cleanup(BelaContext *context, void *userData)
{
	if (gState) {
		gen_exported::destroy(gState);
	}
}
