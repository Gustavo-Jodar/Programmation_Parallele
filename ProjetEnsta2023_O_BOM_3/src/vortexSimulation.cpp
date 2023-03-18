#include <SFML/Window/Keyboard.hpp>
#include <ios>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>
#include <chrono>
#include "cartesian_grid_of_speed.hpp"
#include "vortex.hpp"
#include "cloud_of_points.hpp"
#include "runge_kutta.hpp"
#include "screen.hpp"
#include <mpi.h>

// prof's function to read config
auto readConfigFile(std::ifstream &input)
{
    using point = Simulation::Vortices::point;

    int isMobile;
    std::size_t nbVortices;
    Numeric::CartesianGridOfSpeed cartesianGrid;
    Geometry::CloudOfPoints cloudOfPoints;
    constexpr std::size_t maxBuffer = 8192;
    char buffer[maxBuffer];
    std::string sbuffer;
    std::stringstream ibuffer;
    // Lit la première ligne de commentaire :
    input.getline(buffer, maxBuffer); // Relit un commentaire
    input.getline(buffer, maxBuffer); // Lecture de la grille cartésienne
    sbuffer = std::string(buffer, maxBuffer);
    ibuffer = std::stringstream(sbuffer);
    double xleft, ybot, h;
    std::size_t nx, ny;
    ibuffer >> xleft >> ybot >> nx >> ny >> h;
    cartesianGrid = Numeric::CartesianGridOfSpeed({nx, ny}, point{xleft, ybot}, h);
    input.getline(buffer, maxBuffer); // Relit un commentaire
    input.getline(buffer, maxBuffer); // Lit mode de génération des particules
    sbuffer = std::string(buffer, maxBuffer);
    ibuffer = std::stringstream(sbuffer);
    int modeGeneration;
    ibuffer >> modeGeneration;
    if (modeGeneration == 0) // Génération sur toute la grille
    {
        std::size_t nbPoints;
        ibuffer >> nbPoints;
        cloudOfPoints = Geometry::generatePointsIn(nbPoints, {cartesianGrid.getLeftBottomVertex(), cartesianGrid.getRightTopVertex()});
    }
    else
    {
        std::size_t nbPoints;
        double xl, xr, yb, yt;
        ibuffer >> xl >> yb >> xr >> yt >> nbPoints;
        cloudOfPoints = Geometry::generatePointsIn(nbPoints, {point{xl, yb}, point{xr, yt}});
    }
    // Lit le nombre de vortex :
    input.getline(buffer, maxBuffer); // Relit un commentaire
    input.getline(buffer, maxBuffer); // Lit le nombre de vortex
    sbuffer = std::string(buffer, maxBuffer);
    ibuffer = std::stringstream(sbuffer);
    try
    {
        ibuffer >> nbVortices;
    }
    catch (std::ios_base::failure &err)
    {
        std::cout << "Error " << err.what() << " found" << std::endl;
        std::cout << "Read line : " << sbuffer << std::endl;
        throw err;
    }
    Simulation::Vortices vortices(nbVortices, {cartesianGrid.getLeftBottomVertex(),
                                               cartesianGrid.getRightTopVertex()});
    input.getline(buffer, maxBuffer); // Relit un commentaire
    for (std::size_t iVortex = 0; iVortex < nbVortices; ++iVortex)
    {
        input.getline(buffer, maxBuffer);
        double x, y, force;
        std::string sbuffer(buffer, maxBuffer);
        std::stringstream ibuffer(sbuffer);
        ibuffer >> x >> y >> force;
        vortices.setVortex(iVortex, point{x, y}, force);
    }
    input.getline(buffer, maxBuffer); // Relit un commentaire
    input.getline(buffer, maxBuffer); // Lit le mode de déplacement des vortex
    sbuffer = std::string(buffer, maxBuffer);
    ibuffer = std::stringstream(sbuffer);
    ibuffer >> isMobile;
    return std::make_tuple(vortices, isMobile, cartesianGrid, cloudOfPoints);
}

int main(int nargs, char *argv[])
{

    // number of process and process number
    int nb_process, num_proc;

    // initializing MPI
    MPI_Init(&nargs, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_process);
    MPI_Comm_rank(MPI_COMM_WORLD, &num_proc);

    // check if has arguments for configuration
    char const *filename;
    if (nargs == 1)
    {
        std::cout << "Usage : vortexsimulator <nom fichier configuration>" << std::endl;
        return EXIT_FAILURE;
    }

    // using configs
    filename = argv[1];
    std::ifstream fich(filename);
    auto config = readConfigFile(fich);
    fich.close();

    std::size_t resx = 800, resy = 600;
    if (nargs > 3)
    {
        resx = std::stoull(argv[2]);
        resy = std::stoull(argv[3]);
    }

    // initializing variables
    auto vortices = std::get<0>(config);
    auto isMobile = std::get<1>(config);
    auto grid = std::get<2>(config);
    auto cloud = std::get<3>(config);

    grid.updateVelocityField(vortices);

    bool animate = true;

    double dt = 0.1; // velocity

    MPI_Status status; // mpi_status for MPI_Iprobe
    
    //creating partial datas
    int partial_cloud_size = cloud.numberOfPoints() / (nb_process-1);

    //things for gather cloud
    int dspls[nb_process], recv_counts[nb_process];
    dspls[0] = 0;
    recv_counts[0] = 0;
    for (int i = 1; i < nb_process; i++)
    {
        recv_counts[i] = 2* partial_cloud_size;
        dspls[i] = 2 * partial_cloud_size * (i-1);
    }

    if (num_proc == 0)
    {
        // screen info
        Graphisme::Screen myScreen({resx, resy}, {grid.getLeftBottomVertex(), grid.getRightTopVertex()});
        std::cout << "######## Vortex simultor ########" << std::endl
                  << std::endl;
        std::cout << "Press P for play animation " << std::endl;
        std::cout << "Press S to stop animation" << std::endl;
        std::cout << "Press right cursor to advance step by step in time" << std::endl;
        std::cout << "Press down cursor to halve the time step" << std::endl;
        std::cout << "Press up cursor to double the time step" << std::endl;

        // initializing variable to hold keyboard's commands
        char key = '_';
        auto start = std::chrono::system_clock::now();
        int frames = 0;
        int fps = 0;
        while (myScreen.isOpen())
        {

            key = '_';
            // on inspecte tous les évènements de la fenêtre qui ont été émis depuis la précédente itération
            sf::Event event;
            // while inspecting screen and the command is not T (terminate other processes) or X (terminate this process)
            while (myScreen.pollEvent(event) && key != 'T' && key != 'X')
            {
                // event resize screen
                if (event.type == sf::Event::Resized)
                    myScreen.resize(event);
                // event play animation
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::P))
                    key = 'P';
                // event stop animation
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                    key = 'S';
                // event +speed animation
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                {
                    key = 'U';
                    dt *= 2;
                }
                // event -speed animatin
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                {
                    key = 'D';
                    dt /= 2;
                }
                // event advance
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                    key = 'R';
                // event close window and terminate other processes
                if (event.type == sf::Event::Closed)
                    key = 'T';
                // event close window and terminate other processes
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)){
                    key = 'T';
                }
                // if a key is pressed, send this key to process 1
                if (key != '_'){
                    for(int i = 1; i < nb_process; i++){
                        MPI_Send(&key, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
                    }
                    if(key == 'T'){
                        myScreen.close();
                        MPI_Finalize();
                        return EXIT_SUCCESS;
                    }
                }
            }

            key = 'N';
            for(int i = 1; i < nb_process; i++){
                MPI_Send(&key, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            }
            // receiving calculated data

            MPI_Gatherv(cloud.data(), 2* partial_cloud_size, MPI_DOUBLE, cloud.data(), recv_counts, dspls, MPI_DOUBLE, 0 , MPI_COMM_WORLD);
            
            MPI_Recv(grid.data(), 2 * grid.get_size_velocity_field(), MPI_DOUBLE, 1, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(vortices.data(), vortices.numberOfVortices() * 3, MPI_DOUBLE, 1, 3, MPI_COMM_WORLD, &status);
                    
            frames++;

            // updating screen
            myScreen.clear(sf::Color::Black);

            std::string strDt = std::string("Time step : ") + std::to_string(dt);
            myScreen.drawText(strDt, Geometry::Point<double>{50, double(myScreen.getGeometry().second - 96)});
            auto start_af = std::chrono::system_clock::now();
            
            myScreen.displayVelocityField(grid, vortices);
            auto end_af = std::chrono::system_clock::now();
            std::chrono::duration<double> diff_af = end_af - start_af;
            //std::cout << "aff = " << diff_af.count() << std::endl;
            
            myScreen.displayParticles(grid, vortices, cloud);

            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> diff = end - start;
            if (diff.count() >= 1.0)
            {
                fps = frames;
                start = end;
                frames = 0;
            }
            std::string str_fps = std::string("FPS : ") + std::to_string(fps);
            myScreen.drawText(str_fps, Geometry::Point<double>{300, double(myScreen.getGeometry().second - 96)});
            
            myScreen.display();
            
            
        }
    }

    if (num_proc != 0)
    {
        //making specific cloud of points
        Geometry::CloudOfPoints specific_cloud(partial_cloud_size);

        for (int iPoint = 0; iPoint < partial_cloud_size; iPoint++) {
            specific_cloud[iPoint] = cloud[partial_cloud_size * (num_proc - 1) + iPoint];
        }        

        while (true)
        {
            bool advance = false;
            bool calculate = false;
            char key = '_';
            
            // reading keyboard pressed
            MPI_Recv(&key, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
            switch (key)
            {
            case 'T': // terminate processes
                MPI_Finalize();
                return EXIT_SUCCESS;
            case 'P': // play
                animate = true;
                break;
            case 'S': // stop
                animate = false;
                break;
            case 'U': //+speed
                dt *= 2;
                break;
            case 'D': //-speed
                dt /= 2;
                break;
            case 'R': // advance
                advance = true;
                calculate = true;
                break;
            case 'N':
                calculate = true;
                break;
            default:
                break;
            }

            
            if(calculate){
                if (animate | advance)
                {
                    
                    specific_cloud = Numeric::solve_RK4_fixed_vortices(dt, grid, specific_cloud);            
                    if(isMobile){
                        Numeric::update_vortices(dt, grid, vortices, specific_cloud);
                    }
                    MPI_Gatherv(specific_cloud.data(), 2* partial_cloud_size, MPI_DOUBLE, cloud.data(), recv_counts, dspls,MPI_DOUBLE,0 , MPI_COMM_WORLD);
                    
                    if(num_proc == 1){
                        MPI_Send(grid.data(), 2 * grid.get_size_velocity_field(), MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
                        MPI_Send(vortices.data(), vortices.numberOfVortices() * 3, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD);
                    }
                }
            }
            
        }
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
