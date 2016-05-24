#include <iostream>

#include <pcl/console/parse.h>

#include "logger.h"
#include "defaults.h"
#include "parameter.h"
#include "pointcloud.h"
#include "visualizer.h"
#include "pipeline.hpp"
#include "configuration.hpp"
#include "optimization/optimizer.hpp"

using namespace PoseEstimation;

void showHelp(char *appname)
{
    std::cout << std::endl;
    std::cout << "***************************************************************************" << std::endl;
    std::cout << "*                                                                         *" << std::endl;
    std::cout << "*                             Pose Estimation                             *" << std::endl;
    std::cout << "*                                                                         *" << std::endl;
    std::cout << "***************************************************************************" << std::endl;
    std::cout << "Usage: " << *appname << " source_filename.pcd target_filename.pcd" << std::endl << std::endl;
    Parameter::displayAll();
}

int main(int argc, char **argv)
{
    Parameter::parseAll(argc, argv);

    Configuration::argumentCategory.parameters();
    Parameter::saveAll();
    Parameter::loadAll();

    Visualizer::enabled() = false;

    if (pcl::console::find_switch(argc, argv, "-h"))
        showHelp(argv[0]);

    // source & target pcl filenames
    std::vector<int> filenames = pcl::console::parse_file_extension_argument(argc, argv, ".pcd");
    if (filenames.size() != 2)
    {
        Logger::error("Filenames missing.");
        showHelp(argv[0]);
        std::exit(EXIT_FAILURE);
    }

    std::string source_filename = argv[filenames[0]];
    std::string target_filename = argv[filenames[1]];
    PointCloud source, target;
    if (!PointCloud::loadFromFile(source_filename, source))
    {
        Logger::error(boost::format("Couldn't load source cloud from \"%s\".") % source_filename);
        std::exit(EXIT_FAILURE);
    }
    if (!PointCloud::loadFromFile(target_filename, target))
    {
        Logger::error(boost::format("Couldn't load target cloud from \"%s\".") % target_filename);
        std::exit(EXIT_FAILURE);
    }

    // move target cloud to the right to visualize source & target side by side
    target.translate(1, 0, 0);

    Configuration config;
    config.useModule(PipelineModuleType::HypothesisVerifier, false);

    Optimizer opt(source, target);
    OptimizationResult res = opt.optimize(config);

    for (auto &assignment : res.bestAssignment)
    {
        Parameter::get(assignment.first)->setNumericalValue(assignment.second);
        Logger::debug(boost::format("Setting %d = %d") % assignment.first % assignment.second);
    }

    Visualizer::enabled() = true;

    //config.useModule(PipelineModuleType::HypothesisVerifier, true);
    // actual pose estimation pipeline
    config.run(source, target);

    Visualizer::visualize(source);
    Visualizer::visualize(target);

    Visualizer::render();

    return 0;
}
