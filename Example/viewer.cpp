/*
 * Autonomous Robotics & Perception Group
 * The George Washington University
 * http://rpg.robotics.gwu.edu/
 *
 */

#include <pangolin/pangolin.h>				// pangolin header
#include <SceneGraph/SceneGraph.h>			// scenegraph header

#include <RPG/Utils/GetPot>				// useful command line parser
#include <RPG/Utils/InitCam.h>				// camera initializer
#include <RPG/Utils/ImageWrapper.h>			// image class
#include <RPG/Devices/Camera/CameraDevice.h>		// camera device class

int main( int argc, char** argv )
{
    // parse parameters
    GetPot cl( argc, argv );

    // create new generic camera
    // this camera will be initialized to whatever device the user specifies through command line
    CameraDevice    Cam;

    // initialize camera
    // it is VERY important to read what this function does, since some cameras require
    // certain options in order to be initialized correctly. The options are listed in the
    // InitCam.h file and are passed through command line.
    rpg::InitCam( Cam, cl );

    // vector of images which will hold images captured by camera
    std::vector< rpg::ImageWrapper > vImages;

    // capture an initial image to get image params
    Cam.Capture( vImages );

    // image info
    const unsigned int nNumImgs     = vImages.size();
    const unsigned int nImgHeight   = vImages[0].Image.rows;	// we assume all images have same height
    const unsigned int nImgWidth    = vImages[0].Image.cols;    // and width

    if( nNumImgs == 0 ) {
        std::cerr << "No images found!" << std::endl;
        exit(0);
    }

    // Create OpenGL window in single line thanks to GLUT
    pangolin::CreateGlutWindowAndBind( "Camera Viewer", 1200, 600 );
    SceneGraph::GLSceneGraph::ApplyPreferredGlSettings();

    // Pangolin abstracts the OpenGL viewport as a View.
    // Here we get a reference to the default 'base' view.
    pangolin::View& glBaseView = pangolin::DisplayBase();

    // prepare image holders
    SceneGraph::ImageView			 glLeftImg;
    SceneGraph::ImageView			 glRightImg;
    glLeftImg.SetBounds( 0.0, 1.0, 0.0, 0.5, (double)nImgWidth / nImgHeight );
    glRightImg.SetBounds( 0.0, 1.0, 0.5, 1.0, (double)nImgWidth / nImgHeight );

    // Add our views as children to the base container.
    glBaseView.AddDisplay( glLeftImg );
    glBaseView.AddDisplay( glRightImg );

    // Default hooks for exiting (Esc) and fullscreen (tab).
    while( !pangolin::ShouldQuit() ) {

        // clear whole screen
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        // capture an images if not logging
        Cam.Capture(vImages);

        // show left image - RGB data
        glLeftImg.SetImage( vImages[0].Image.data, nImgWidth, nImgHeight, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE );

        // show right image - depth information
        glRightImg.SetImage( vImages[1].Image.data, nImgWidth, nImgHeight, GL_INTENSITY, GL_LUMINANCE, GL_UNSIGNED_SHORT );

        // render screen
        pangolin::FinishGlutFrame();

    }

    return 0;
}
