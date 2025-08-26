use lang;

// tellraw impl used by msg, and debug::print

object text_component
{
    
}

method: void tellraw(const __p: selector, const __t: text_component&) __cpp__;


const player: selector = @r;
method: void onXClick()
{
    msg(player, "Stupid");
}
xText: string = "Click me if you are stupid.";

const x: text_component = new text_component(color ="#FF0000",
                       bold  =true,
                       italic=true,
                       onClick=onXClick,
                       text=xText,
                       extra=[y,z,w,...]);

tellraw(player, x);
