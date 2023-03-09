use core::time;
use crossterm::terminal::{Clear, ClearType};
use ndarray::{arr2, s, Array2};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::error::Error;
use std::fs::File;
use std::io::BufReader;
use std::thread;

pub trait Collidable {}

#[derive(Debug)]
struct Coordinate {
    x: i32,
    y: i32,
}

#[derive(Debug)]
struct ObjDimension {
    x: usize,
    y: usize,
}

type Region = Array2<char>;

#[derive(Debug)]
struct Object {
    placement: Coordinate,
    visual: Region,
    size: ObjDimension,
    layer: usize,
}

impl Object {
    fn new(visual_path: String) -> Result<Object, Box<dyn Error>> {
        // let file = File::open(visual_path)?;
        // let reader = BufReader::new(file);
        // let visual: Array2<char> = serde_json::from_reader(reader).unwrap();

        let visual: Region = serde_json::from_str(&visual_path).unwrap();
        let size = ObjDimension {
            x: visual.shape()[0],
            y: visual.shape()[1],
        };

        Ok(Object {
            placement: Coordinate { x: 0, y: 0 },
            visual,
            size,
            layer: 0,
        })
    }
}

#[derive(Debug)]
struct Scene {
    object: HashMap<String, Box<Object>>,
    pub frame: Region,
}

impl Scene {
    fn new(object: HashMap<String, Box<Object>>, resolution: ObjDimension) -> Scene {
        let frame = Array2::<char>::default((resolution.x, resolution.y));
        Scene { object, frame }
    }

    fn render(&mut self) {
        // self.frame = Array2::<char>::default((10, 10));
        for (_, obj) in &mut self.object {
            let asd = obj.visual.shape()[0];

            let x_start = if obj.placement.x < 0 {
                0
            } else {
                obj.placement.x
            };

            let x_end = if obj.placement.x < 0 {
                obj.size.x - obj.placement.x.abs() as usize
            } else {
                obj.size.x
            };

            let y_start = if obj.placement.y < 0 {
                0
            } else {
                obj.placement.y
            };

            let y_end = if obj.placement.y < 0 {
                obj.size.y - obj.placement.y.abs() as usize
            } else {
                obj.size.y
            };

            let mut obj_region = self
                .frame
                .slice_mut(s![x_start..x_end as i32, y_start..y_end as i32]);
            obj_region = obj.visual.view_mut();
        }
    }
}

struct App {
    scene: HashMap<String, Box<Scene>>,
    current_scene: String,
    framerate: usize,
    frametime: u64,
    resolution: ObjDimension,
}

impl App {
    fn new(framerate: usize, resolution: ObjDimension, scene: HashMap<String, Box<Scene>>) -> App {
        App {
            scene,
            current_scene: "test_scene".to_owned(),
            framerate,
            frametime: 1000 / framerate as u64,
            resolution,
        }
    }

    fn display(&self) {
        // print!("{:?}", self.scene[&self.current_scene]);
        for char in &self.scene[&self.current_scene].frame {
            if char != &'\0' {
                print!("{} ", char);
            } else {
                print!("_ ");
            }
        }
    }

    fn start(&mut self) -> () {
        loop {
            self.display();
            thread::sleep(time::Duration::from_millis(self.frametime));
            Clear(ClearType::All);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let cube = arr2(&[['X', 'X'], ['X', 'X']]);
        let serialized_cube = serde_json::to_string(&cube).unwrap();

        // let test_obj = Object::new("Cube".to_owned()).unwrap();
        let test_obj = Object::new(serialized_cube).unwrap();
        let mut obj_list = HashMap::new();
        obj_list.insert("Cube".to_owned(), Box::new(test_obj));

        let test_scene = Scene::new(obj_list, ObjDimension { x: 40, y: 30 });
        let mut scene_list = HashMap::new();
        scene_list.insert("test_scene".to_owned(), Box::new(test_scene));

        let mut test_game = App::new(3, ObjDimension { x: 40, y: 30 }, scene_list);

        test_game.start();
    }
}
